#include "VMTHook.hpp"
#include "Hooks.hpp"
#include "Chams.hpp"
#include "D3DShaderDebugger.hpp"
#include "FontManager.hpp"

#include "../Blam/BlamObjects.hpp"
#include "../Blam/BlamPlayers.hpp"
#include "../Blam/Tags/TagInstance.hpp"
#include "../Patch.hpp"

#include <detours.h>

#include <stdio.h>  
#include <intrin.h>
#include <fstream>
#include <algorithm>
#include <mutex>
#include <map>

#include <boost\filesystem.hpp>

#define D3D_BEGINSCENE_INDEX 41
#define D3D_DRAWINDEXEDPRIMITIVE_INDEX 82

using namespace Blam;
using namespace Pringle;

namespace
{
	struct PositionCompressionScaleData
	{
		uint32_t Type; // seems to be mostly 3, which may indicate the vector count (?)
		float f4;
		float f8;
		float fC;
		float f10;
		float f14;
		float f18;
		float f1C;
		float f20;
		float f24;
		float f28;

		/*
			SetVertexShaderConstantF(12, {}, 3):
			{
				// vector4f 1:
				float m0 = f24;
				float m1 = f1C;
				float m2 = f28 - f24;
				float m3 = f20 - f1C;
				// vector4f 2:
				float m4 = 0;
				float m5 = f0x14;
				float m6 = f0C;
				float m7 = f4;
				// vector4f 3:
				float m8 = 1.f;
				float m9 = f18 - f14;
				float m10 = f10 - f0C;
				float m11 = f8 - f4;
			}
		*/
	};
	static_assert(sizeof(PositionCompressionScaleData) == 0x2C);

	struct VertexNodes
	{
		uint8_t unknown0[0x44];
		float f44; // uploaded vertex data for register 16
		float f48;
		float f4C;
		float f50;
		float f54;
		float f58;
		float f5C;
		float f60;
		float f64;
		float f68;
		float f6C;
		float f70;
		uint8_t unknown[0x30];
	};
	static_assert(sizeof(VertexNodes) == 0xA4);

	struct VertexShader16Info
	{
		uint8_t unknown0[0x2E];
		uint16_t VertexCountUnknown; // if this is 2 or 22 then it has more than 3 vector4fs in its shader data
	};

	struct VertexTagData
	{
		uint8_t unknown[0x6C];
		void* BaseAddress; // base address of something
		uint8_t unknown0[0x8];
		PositionCompressionScaleData* ScaleData;

		inline VertexShader16Info* GetVertexShader16Info(uint16_t index)
		{
			return reinterpret_cast<VertexShader16Info*>(reinterpret_cast<uint8_t*>(this->BaseAddress) + (index * 0x4C));
		}
	};

	struct RenderData
	{
		uint32_t Unknown0;
		uint32_t TagIndex;
		uint32_t VertexShader16DataOffset;
		uint8_t Unknown[0x58];
		bool ShouldClipPlane;
		uint8_t Pad65[3];
		uint32_t unknown68;
		uint32_t ObjectHandle;

		inline Objects::ObjectHeader* GetRenderObject()
		{
			return Objects::GetObjects().Get(this->ObjectHandle);
		}

		inline VertexNodes* GetVertexNodes() // this is wrong sometimes
		{
			return reinterpret_cast<VertexNodes*(__cdecl*)(uint32_t)>(0x00A6DAA0)(this->VertexShader16DataOffset);
		}

		inline VertexTagData* GetTagInstance()
		{
			return reinterpret_cast<VertexTagData*(__cdecl*)(int, uint32_t)>(0x503370)(0x6D6F6465, this->TagIndex); // tag index should be 13754
		}
	};

	struct RenderHeader
	{
		uint32_t unknown0;	// some render flag, AND'ed against the second argument of function 0xA78A50, possibly to stop some segments from rendering in certain modes
							// used again to AND'd with 0x8, if the result is true, sub_A786B0 is called instead of our hook  
		RenderData* Data;
		uint32_t unknown8; // used for sub_A786B0
		uint16_t unknown12;
		uint16_t VertexShader16InfoIndex; // always seems to be 1 or 0. only seems to change to 1 after first segment. possibly a bool with +bytes of padding
		uint16_t VertexShader16Index; // increments by 1 after 2nd segment is set to 1 (starts at 0 the first time tho), possibly index for the previous field
		uint32_t SegmentIndex;
		uint32_t unknown18; // always zero?
	};
	static_assert(sizeof(RenderHeader) == 0x1C);

	struct PrimitiveData // first argument of 0xA28270
	{
		int PrimitiveType;
		int Unknown4;
		void* IndexPtr;
	};

	struct Cluster
	{
		uint16_t Offset;
		uint16_t Unknown2;
		uint8_t unknown8[0x16];
	};
	static_assert(sizeof(Cluster) == 0x1A);

	static bool IsInternalCall_BeginScene = false;
	static bool IsInternalCall_DrawIndexedPrimitive = false;

	class CallScopeGuard
	{
		bool& Value;
	public:
		CallScopeGuard(bool& value) : Value(value)
		{
			Value = true;
		}
		~CallScopeGuard()
		{
			Value = false;
		}
	};

	static Objects::ObjectHeader* CurrentObject_ptr;

	class ObjectScopeGuard
	{
	public:
		ObjectScopeGuard(Objects::ObjectHeader* object)
		{
			CurrentObject_ptr = object;
		}
		~ObjectScopeGuard()
		{
			CurrentObject_ptr = nullptr;
		}
	};
}

namespace
{
	typedef long(__stdcall* BeginScene_t)(void*);
	typedef long(__stdcall* DrawIndexedPrimitive_t)(void*, int, int, unsigned int, unsigned int, unsigned int, unsigned int);
	typedef bool(__cdecl* RenderSegment_t)(RenderHeader*, int);

	auto BeginScene_ptr = reinterpret_cast<BeginScene_t>(0x00);
	auto DrawIndexedPrimitive_ptr = reinterpret_cast<DrawIndexedPrimitive_t>(0x00);
	auto RenderSegment_ptr = reinterpret_cast<RenderSegment_t>(0xA78940);

	HRESULT __stdcall BeginScene_hook(LPDIRECT3DDEVICE9 device)
	{
		if (!IsInternalCall_BeginScene)
		{
			CallScopeGuard guard(IsInternalCall_BeginScene);
			Pringle::Hook::Call<Hooks::DirectX::BeginScene>(device);
		}
		return BeginScene_ptr(device);
	}

	HRESULT __stdcall DrawIndexedPrimitive_hook(LPDIRECT3DDEVICE9 device, D3DPRIMITIVETYPE type, int BaseVertexIndex, unsigned int MinVertexIndex, unsigned int NumVertices, unsigned int startIndex, unsigned int primCount)
	{
		if (!IsInternalCall_DrawIndexedPrimitive)
		{
			CallScopeGuard guard(IsInternalCall_DrawIndexedPrimitive); // prevent endless loops

			if (CurrentObject_ptr)
			{
				bool ShouldCall = true;
				Pringle::Hook::Call<Hooks::DirectX::DrawObjectPrimitive>(device, CurrentObject_ptr, ShouldCall);
				if (!ShouldCall) 
					return S_OK;
			}
		}
		return DrawIndexedPrimitive_ptr(device, type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
	}

	bool __cdecl RenderSegment_hook(RenderHeader* header, int type)
	{
		if (header && header->Data)
		{
			auto obj = header->Data->GetRenderObject();
			if (obj)
			{
				ObjectScopeGuard guard(obj);
				return RenderSegment_ptr(header, type);
			}
		}
		return RenderSegment_ptr(header, type);
	}
}

class HookInitializer
{
public:
	HookInitializer()
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(reinterpret_cast<void**>(&RenderSegment_ptr), &RenderSegment_hook);
		DetourTransactionCommit();

		Pringle::Hook::SubscribeMember<Pringle::Hooks::DirectX::Initialize>(this, &HookInitializer::OnInitialize);
	}

	void OnInitialize(const Pringle::Hooks::DirectX::Initialize& msg)
	{
		auto vtable = reinterpret_cast<uint32_t*>(*reinterpret_cast<uint32_t*>(msg.Device));
		BeginScene_ptr = reinterpret_cast<BeginScene_t>(vtable[D3D_BEGINSCENE_INDEX]);
		DrawIndexedPrimitive_ptr = reinterpret_cast<DrawIndexedPrimitive_t>(vtable[D3D_DRAWINDEXEDPRIMITIVE_INDEX]);

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(reinterpret_cast<void**>(&BeginScene_ptr), &BeginScene_hook);
		DetourAttach(reinterpret_cast<void**>(&DrawIndexedPrimitive_ptr), &DrawIndexedPrimitive_hook);
		DetourTransactionCommit();
	}
} _; // this will initialize the class