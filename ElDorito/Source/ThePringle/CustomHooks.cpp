#include "CustomHooks.hpp"
#include "Hooks.hpp"
#include "Chams.hpp"
#include "D3DShaderDebugger.hpp"
#include "FontManager.hpp"

#include "../Blam/BlamObjects.hpp"
#include "../Blam/BlamPlayers.hpp"
#include "../Blam/Tags/TagInstance.hpp"

#include <detours.h>

#include <stdio.h>  
#include <intrin.h>
#include <fstream>
#include <algorithm>
#include <mutex>
#include <map>

#include <boost\filesystem.hpp>

#pragma intrinsic(_ReturnAddress)

#define D3D_CLEAR_INDEX 43
#define D3D_DRAWINDEXEDPRIMITIVE_INDEX 82
#define D3D_CREATEVERTEXSHADER_INDEX 91
#define D3D_CREATEPIXELSHADER_INDEX 106
#define D3D_CREATEQUERY_INDEX 118

//#define DUMP_SHADERS

using namespace Blam;

namespace
{
	static Objects::ObjectHeader* GetPlayerObjectHeader(Players::PlayerDatum* pl, bool includeDead = false)
	{
		if (!pl)
			return nullptr;

		if (!pl->SlaveUnit && (!includeDead || !pl->DeadSlaveUnit))
			return nullptr;

		auto header = Objects::GetObjects().Get(pl->SlaveUnit);
		if (!header || !header->Data)
		{
			if (includeDead)
			{
				header = Objects::GetObjects().Get(pl->DeadSlaveUnit);

				if (!header || !header->Data)
					return nullptr;
			}
			else
				return nullptr;
		}

		return header;
	}

	struct PlayerInfo
	{
		Players::PlayerDatum* Datum;
		Objects::ObjectHeader* Header;

		bool IsNull()
		{
			return Datum == nullptr || Header == nullptr;
		}

		PlayerInfo() : Datum(nullptr), Header(nullptr) { };
		PlayerInfo(Objects::ObjectHeader* header) : PlayerInfo()
		{
			if (header->Data)
			{
				auto index = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(header->Data) + 0x198);
				if (index != -1)
				{
					auto pl = Players::GetPlayers().Get(index);
					if (pl)
					{
						this->Datum = pl;
						this->Header = header;
					}
				}
			}
		}
	};

	static PlayerInfo GetLocalPlayer(bool includeDead = false)
	{
		PlayerInfo info;

		auto& handle = Players::GetLocalPlayer(0);
		if (handle == DatumHandle::Null)
			return info;

		auto pl = Players::GetPlayers().Get(handle);
		if (!pl)
			return info;

		info.Datum = pl;
		info.Header = GetPlayerObjectHeader(pl, includeDead);

		return info;
	}

	struct VertexShader12Data
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
	static_assert(sizeof(VertexShader12Data) == 0x2C);

	struct VertexShader16Data
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
	static_assert(sizeof(VertexShader16Data) == 0xA4);

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
		VertexShader12Data* VertexShader12Data;

		VertexShader16Info* GetVertexShader16Info(uint16_t index)
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
		uint32_t ObjectIndex;

		VertexShader16Data* GetVertexShader16Data() // this is wrong sometimes
		{
			return reinterpret_cast<VertexShader16Data*(__cdecl*)(uint32_t)>(0x00A6DAA0)(this->VertexShader16DataOffset);
		}

		VertexTagData* GetTagInstance()
		{
			return reinterpret_cast<VertexTagData*(__cdecl*)(int, uint32_t)>(0x503370)(0x6D6F6465, this->TagIndex); // tag index should be 13754
		}
	};

	struct RenderHeader
	{
		uint32_t unknown0;
		RenderData* Data;
		uint8_t unknown8[0x6];
		uint16_t VertexShader16InfoIndex;
		uint16_t VertexShader16Index;
	};

	struct PrimitiveData // first argument of 0xA28270
	{
		int PrimitiveType;
		int Unknown4;
		void* IndexPtr;
	};

	static PlayerInfo LocalPlayer;
	static std::mutex localplayer_mutex;

	static std::vector<uint16_t> player_count;
}

namespace Pringle::CustomHooks
{
	typedef long(__stdcall* DrawIndexedPrimitive_t)(void*, int, int, unsigned int, unsigned int, unsigned int, unsigned int);
	typedef long(__stdcall* CreateQuery_t)(void*, int, IDirect3DQuery9**);
	typedef long(__stdcall* Clear_t)(void*, long, const void*, long, long, float, long);

	typedef long(__stdcall* CreateVertexShader_t)(void*, const DWORD*, IDirect3DVertexShader9**);
	typedef long(__stdcall* CreatePixelShader_t)(void*, const DWORD*, IDirect3DPixelShader9**);

	DrawIndexedPrimitive_t DrawIndexedPrimitive_ptr = 0;
	CreateQuery_t CreateQuery_ptr = 0;
	Clear_t Clear_ptr = 0;
	CreateVertexShader_t CreateVertexShader_ptr = 0;
	CreatePixelShader_t CreatePixelShader_ptr = 0;

	LPDIRECT3DPIXELSHADER9 PixelShader_Chams;

	typedef bool(__cdecl* RenderSegment_t)(RenderHeader*, int);

	auto RenderSegment_ptr = reinterpret_cast<RenderSegment_t>(0xA78940);

	const auto D3DDevice_ptr = reinterpret_cast<LPDIRECT3DDEVICE9*>(0x50DADDC);
	const auto D3DDeviceStartup_ptr = reinterpret_cast<LPDIRECT3DDEVICE9*>(0x0524B6AC); // both point to the same device
	const auto D3DNeedsReset_ptr = reinterpret_cast<bool*>(0x5106FAF);
	const auto D3DUploadShadersLater_ptr = reinterpret_cast<bool*>(0x191808C);

	const auto SetRenderDepthMode_ptr = reinterpret_cast<int(__cdecl*)(int)>(0xA247E0);
	const auto CurrentRenderDepthMode_ptr = reinterpret_cast<int*>(0x50DADFC);

	const PlayerInfo NullPlayerInfo;
	//PlayerInfo* Current_Player;
	std::unique_ptr<PlayerInfo> Current_Player;

	//https://msdn.microsoft.com/en-us/library/windows/desktop/bb219845(v=vs.85).aspx
	HRESULT PixelShaderOverride(LPDIRECT3DPIXELSHADER9 shader)
	{
		return 0; // TODO maybe
	}

	HRESULT CreateShader(LPDIRECT3DDEVICE9 device, LPDIRECT3DPIXELSHADER9* shader)
	{
		static const char PS_COLOR_DEPTH[] = "ps_3_0\nmov oC0, c199\nmov oDepth, c223.r"; // unused registers

		LPD3DXBUFFER buffer = NULL;

		auto err = S_OK;
		if (FAILED(err = D3DXAssembleShader(PS_COLOR_DEPTH, sizeof(PS_COLOR_DEPTH), NULL, NULL, NULL, &buffer, NULL)))
			return err;

		if (FAILED(err = device->CreatePixelShader(reinterpret_cast<DWORD*>(buffer->GetBufferPointer()), shader)))
			return err;

		buffer->Release();

		return S_OK;
	}

	HRESULT __stdcall DrawIndexedPrimitive_Hook(LPDIRECT3DDEVICE9 device, int type, int BaseVertexIndex, unsigned int MinVertexIndex, unsigned int NumVertices, unsigned int startIndex, unsigned int primCount)
	{
		if (Current_Player)
		{
			float color[4] = { 0.f, 0.f, 0.f, 1.f };
			float depth[4] = { 0.f, 0.f, 0.f, 0.f };

			{
				std::lock_guard<std::mutex> lock(localplayer_mutex);

				auto& player = *Current_Player;
				auto& self = LocalPlayer;

				if (!self.IsNull() && !player.IsNull())
				{
					if (player.Datum->GetSalt() == self.Datum->GetSalt())
						goto end; // don't render on self
					else if (self.Datum->Properties.TeamIndex == player.Datum->Properties.TeamIndex)
						color[1] = 1.f; // green
					else
						color[0] = 1.f; // red
				}
				else // dead (probably)
				{
					color[0] = 1.f;
					color[1] = 1.f;
					color[2] = 1.f;
				}
			}

			device->SetPixelShader(PixelShader_Chams);

			device->SetPixelShaderConstantF(199, color, 1);
			device->SetPixelShaderConstantF(223, depth, 1);

			auto ret = DrawIndexedPrimitive_ptr(device, type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);

			// keep this gap just in case i need to change render states

			return ret;
		}

	end:
		return DrawIndexedPrimitive_ptr(device, type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
	}

	long __stdcall CreateQuery_hook(LPDIRECT3DDEVICE9 device, D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery)
	{
		if (Type == D3DQUERYTYPE_OCCLUSION) Type = D3DQUERYTYPE_TIMESTAMP;
		return CreateQuery_ptr(device, Type, ppQuery);
	}

	long __stdcall Clear_hook(LPDIRECT3DDEVICE9 device, DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
	{
		return Clear_ptr(device, Count, pRects, Flags, Color, Z, Stencil);
	}

	long __stdcall CreateVertexShader_hook(LPDIRECT3DDEVICE9 device, const DWORD* pFunction, IDirect3DVertexShader9** ppShader)
	{
		volatile const char* breakpoint = "CreateVertexShader_hook";
#ifdef DUMP_SHADERS
		DirectXDebugger::Shaders::DumpShaderToJson("dumps\\vertex_shaders.json", pFunction, &general_vs_checksums);
#endif // DUMP_SHADERS
		return CreateVertexShader_ptr(device, pFunction, ppShader);
	}

	long __stdcall CreatePixelShader_hook(LPDIRECT3DDEVICE9 device, const DWORD* pFunction, IDirect3DPixelShader9** ppShader)
	{
		volatile const char* breakpoint = "CreatePixelShader_hook";
#ifdef DUMP_SHADERS
		DirectXDebugger::Shaders::DumpShaderToJson("dumps\\pixel_shaders.json", pFunction, &general_ps_checksums);
#endif // DUMP_SHADERS
		return CreatePixelShader_ptr(device, pFunction, ppShader);
	}

	void DirectX::Initialize(LPDIRECT3DDEVICE9 device)
	{
		if (FAILED(CreateShader(device, &PixelShader_Chams)))
			return;

		auto vtable = reinterpret_cast<uint32_t*>(*reinterpret_cast<uint32_t*>(device));
		DrawIndexedPrimitive_ptr = reinterpret_cast<DrawIndexedPrimitive_t>(vtable[D3D_DRAWINDEXEDPRIMITIVE_INDEX]);
		CreateQuery_ptr = reinterpret_cast<CreateQuery_t>(vtable[D3D_CREATEQUERY_INDEX]);
		Clear_ptr = reinterpret_cast<Clear_t>(vtable[D3D_CLEAR_INDEX]);
		CreateVertexShader_ptr = reinterpret_cast<CreateVertexShader_t>(vtable[D3D_CREATEVERTEXSHADER_INDEX]);
		CreatePixelShader_ptr = reinterpret_cast<CreatePixelShader_t>(vtable[D3D_CREATEPIXELSHADER_INDEX]);
		
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(reinterpret_cast<void**>(&DrawIndexedPrimitive_ptr), &DrawIndexedPrimitive_Hook);
		DetourAttach(reinterpret_cast<void**>(&CreateVertexShader_ptr), &CreateVertexShader_hook);
		DetourAttach(reinterpret_cast<void**>(&CreatePixelShader_ptr), &CreatePixelShader_hook);
		DetourTransactionCommit();
	}

	bool __cdecl RenderSegment_hook(RenderHeader* header, int type)
	{
		if (header && header->Data)
		{
			auto obj = Blam::Objects::GetObjects().Get(header->Data->ObjectIndex);
			if (!obj) 
				goto end;

			switch (obj->Type)
			{
			case Objects::eObjectTypeScenery:
				obj = Blam::Objects::GetObjects().Get(obj->Data->Parent); // this for the extra armor rendered on the player, apparently its scenery???
				if (!obj || obj->Type != Objects::eObjectTypeBiped)
					goto end;
			case Objects::eObjectTypeBiped:
				break;
			default:
				goto end;
			}

			Current_Player = std::make_unique<PlayerInfo>(obj); // had to do this because it wasn't working in release mode
			auto ret = RenderSegment_ptr(header, type);
			Current_Player = nullptr;

			return ret;
		}
		
		end: return RenderSegment_ptr(header, type);
	}

	void Halo::Initialize()
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(reinterpret_cast<void**>(&RenderSegment_ptr), &RenderSegment_hook);
		DetourTransactionCommit();
	}
}

class HookInitializer
{
public:
	HookInitializer()
	{
		Pringle::CustomHooks::Halo::Initialize();
		Pringle::Hook::SubscribeMember<Pringle::Hooks::DirectX::Initialize>(this, &HookInitializer::OnInitialize);
		//Pringle::Hook::SubscribeMember<Pringle::Hooks::DirectX::EndScene>(this, &HookInitializer::OnEndScene);
		Pringle::Hook::SubscribeMember<Pringle::Hooks::PreTick>(this, &HookInitializer::OnTick);
	}

	void OnInitialize(const Pringle::Hooks::DirectX::Initialize& msg)
	{
		Pringle::CustomHooks::DirectX::Initialize(msg.Device);
	}

	void OnEndScene(const Pringle::Hooks::DirectX::EndScene& msg)
	{
		std::stringstream ss;
		ss << "Players: " << player_count.size();

		msg.Draw->Text(ss.str().c_str(), 10, 20, COLOR4I(255, 255, 255, 255));

		player_count.clear();
	}

	void OnTick(const Pringle::Hooks::PreTick& msg)
	{
		std::lock_guard<std::mutex> lock(localplayer_mutex);
		LocalPlayer = GetLocalPlayer(true);
	}
} _; // this will initialize the class