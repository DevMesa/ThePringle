#include "CustomHooks.hpp"
#include "Hooks.hpp"
#include "Chams.hpp"

#include "../Blam/BlamObjects.hpp"
#include "../Blam/Tags/TagInstance.hpp"

#include <detours.h>

#include <stdio.h>  
#include <intrin.h>  

#pragma intrinsic(_ReturnAddress)

#define D3D_DRAWINDEXEDPRIMITIVE_INDEX 82
#define D3D_CREATEQUERY_INDEX 118

namespace Pringle::CustomHooks
{
	static bool isPlayerRenderCall = false;

	typedef long(__stdcall* DrawIndexedPrimitive_t)(void*, int, int, unsigned int, unsigned int, unsigned int, unsigned int);
	typedef long(__stdcall* CreateQuery_t)(void*, int, IDirect3DQuery9**);

	DrawIndexedPrimitive_t DrawIndexedPrimitive_ptr = 0;
	CreateQuery_t CreateQuery_ptr = 0;

	LPDIRECT3DTEXTURE9 texture_red;

	void MakeTexture(LPDIRECT3DDEVICE9 device, LPDIRECT3DTEXTURE9 texture, uint32_t color)
	{
		if (FAILED(device->CreateTexture(8, 8, 1, D3DUSAGE_DYNAMIC, D3DFMT_A4R4G4B4, D3DPOOL_DEFAULT, &texture, 0)))
			return;

		D3DLOCKED_RECT rect;
		texture->LockRect(0, &rect, 0, 0);

		uint16_t color16 = ((WORD)((color >> 28) & 0xF) << 12)
			| (WORD)(((color >> 20) & 0xF) << 8)
			| (WORD)(((color >> 12) & 0xF) << 4)
			| (WORD)(((color >> 4) & 0xF) << 0);

		auto pcolor = reinterpret_cast<uint16_t*>(rect.pBits);

		for (int i = 0; i < 8 * 8; i++) 
			*pcolor++ = color16;

		texture->UnlockRect(0);
	}

	long __stdcall DrawIndexedPrimitive_Hook(LPDIRECT3DDEVICE9 device, int type, int BaseVertexIndex, unsigned int MinVertexIndex, unsigned int NumVertices, unsigned int startIndex, unsigned int primCount)
	{
		/*
		//auto& cmds = Chams::Instance();
		//if (cmds.Only->ValueInt == 0 ? (NumVertices > cmds.Lower->ValueInt && NumVertices < cmds.Upper->ValueInt) : (cmds.Only->ValueInt == NumVertices))

		LPDIRECT3DVERTEXBUFFER9 stream;
		UINT Offset = 0;
		UINT Stride = 0;

		if (!FAILED(thisptr->GetStreamSource(0, &stream, &Offset, &Stride)) && stream != NULL)
			stream->Release();

		//if(NumVertices == 1008 || NumVertices == 1266 || NumVertices == 996 || NumVertices == 1912)
		if(Stride == 64 && startIndex == 0)
		{
			IDirect3DBaseTexture9* texture;
			thisptr->GetTexture(0, &texture);

			IDirect3DPixelShader9* shader;
			thisptr->GetPixelShader(&shader);

			thisptr->SetRenderState(D3DRS_ZENABLE, FALSE);
			thisptr->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
			thisptr->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
			thisptr->SetRenderState(D3DRS_LIGHTING, false);
			thisptr->SetRenderState(D3DRS_ZFUNC, D3DCMP_NEVER);
			thisptr->SetTexture(0, texture_red);
			thisptr->SetPixelShader(0);
			DrawIndexedPrimitive_ptr(thisptr, type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);

			thisptr->SetTexture(0, texture);
			thisptr->SetPixelShader(shader);
			thisptr->SetRenderState(D3DRS_ZENABLE, TRUE);
			thisptr->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
			thisptr->SetRenderState(D3DRS_LIGHTING, true);
			auto ret = DrawIndexedPrimitive_ptr(thisptr, type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
			return ret;
		}//*/

		if (isPlayerRenderCall)
		{
			/*
			IDirect3DBaseTexture9* texture;
			device->GetTexture(0, &texture);

			IDirect3DPixelShader9* shader;
			device->GetPixelShader(&shader);

			IDirect3DVertexShader9* vshader;
			device->GetVertexShader(&vshader);

			//device->SetTexture(0, texture_red);
			//device->SetPixelShader(0);
			//device->SetVertexShader(0);//*/

			DWORD zfunc, writing, zenable;
			device->GetRenderState(D3DRS_ZENABLE, &zenable);
			device->GetRenderState(D3DRS_ZWRITEENABLE, &writing);
			device->GetRenderState(D3DRS_ZFUNC, &zfunc);

			device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
			device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
			device->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);

			device->SetDepthStencilSurface(0);

			IDirect3DSurface9* surface;
			device->GetDepthStencilSurface(&surface);

			D3DSURFACE_DESC desc;
			surface->GetDesc(&desc);

			// TODO: find out wtf to do

			auto ret = DrawIndexedPrimitive_ptr(device, type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);


			device->SetRenderState(D3DRS_ZENABLE, zenable);
			device->SetRenderState(D3DRS_ZWRITEENABLE, writing);
			device->SetRenderState(D3DRS_ZFUNC, zfunc); // zfunc always is D3DCMP_LESSEQUAL

			/*
			device->SetRenderState(D3DRS_STENCILENABLE, clip);

			device->SetPixelShader(shader);
			device->SetVertexShader(vshader);
			device->SetTexture(0, texture);
			//*/

			return ret;
		}

		return DrawIndexedPrimitive_ptr(device, type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
	}

	long __stdcall CreateQuery_hook(LPDIRECT3DDEVICE9 device, D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery)
	{
		if (Type == D3DQUERYTYPE_OCCLUSION) Type = D3DQUERYTYPE_TIMESTAMP;
		return CreateQuery_ptr(device, Type, ppQuery);
	}

	void DirectX::Initialize(LPDIRECT3DDEVICE9 device)
	{
		MakeTexture(device, texture_red, D3DCOLOR_RGBA(255, 0, 0, 255));

		auto vtable = reinterpret_cast<uint32_t*>(*reinterpret_cast<uint32_t*>(device));
		DrawIndexedPrimitive_ptr = reinterpret_cast<DrawIndexedPrimitive_t>(vtable[D3D_DRAWINDEXEDPRIMITIVE_INDEX]);
		CreateQuery_ptr = reinterpret_cast<CreateQuery_t>(vtable[D3D_CREATEQUERY_INDEX]);
		
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(reinterpret_cast<void**>(&DrawIndexedPrimitive_ptr), &DrawIndexedPrimitive_Hook);
		//DetourAttach(reinterpret_cast<void**>(&CreateQuery_ptr), &CreateQuery_hook);
		DetourTransactionCommit();
	}

	struct RenderData
	{
		uint32_t Unknown0;
		uint32_t TagIndex;
		uint8_t Unknown[0x5C];
		bool ShouldClipPlane;
		uint8_t Pad65[3];
		uint32_t unknown68;
		uint32_t ObjectIndex;
	};

	struct RenderHeader
	{
		uint32_t unknown0;
		RenderData* Data;
		uint8_t unknown8[0x6];
		uint16_t UnknownE; // Offset for Tag?
	};

	struct PrimitiveData // first argument of 0xA28270
	{
		int PrimitiveType;
		int Unknown4;
		void* IndexPtr;
	};

	typedef bool(__cdecl* RenderMesh_t)(RenderHeader*, int);

	auto RenderMesh_ptr = reinterpret_cast<RenderMesh_t>(0xA78940);

	const auto D3DDevice_ptr = reinterpret_cast<LPDIRECT3DDEVICE9*>(0x50DADDC);
	const auto D3DNeedsReset_ptr = reinterpret_cast<bool*>(0x5106FAF);
	const auto D3DUploadShadersLater_ptr = reinterpret_cast<bool*>(0x191808C);

	const auto TagInstance = static_cast<uint32_t>(0x6D6F6465);

	bool __cdecl RenderMesh_hook(RenderHeader* header, int type)
	{
		if (header && header->Data)
		{
			auto obj = Blam::Objects::GetObjects().Get(header->Data->ObjectIndex);
			if (!obj) goto end;

			if (obj->Type != Blam::Objects::eObjectTypeBiped) goto end;

			auto device = *D3DDevice_ptr;
			if (!device) goto end;

			/*
			IDirect3DPixelShader9* shader;
			device->GetPixelShader(&shader);

			IDirect3DBaseTexture9* texture;
			device->GetTexture(0, &texture);
			*/

			//if (type != 1) return false;
			/*
			DWORD zfunc, writing, zenable;
			device->GetRenderState(D3DRS_ZENABLE, &zenable);
			device->GetRenderState(D3DRS_ZWRITEENABLE, &writing);
			device->GetRenderState(D3DRS_ZFUNC, &zfunc);

			device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
			device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
			device->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);//*/

			isPlayerRenderCall = true;
			auto ret = RenderMesh_ptr(header, type);
			isPlayerRenderCall = false;

			/*
			device->SetRenderState(D3DRS_ZENABLE, zenable);
			device->SetRenderState(D3DRS_ZWRITEENABLE, writing);
			device->SetRenderState(D3DRS_ZFUNC, zfunc); // zfunc always is 4 */

			//device->SetPixelShader(shader);
			//device->SetTexture(0, texture);

			return ret;
		}
		
		end: return RenderMesh_ptr(header, type);
	}

	void Halo::Initialize()
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(reinterpret_cast<void**>(&RenderMesh_ptr), &RenderMesh_hook);
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
	}

	void OnInitialize(const Pringle::Hooks::DirectX::Initialize& msg)
	{
		Pringle::CustomHooks::DirectX::Initialize(msg.Device);
	}
};

HookInitializer _; // this will initialize the class on startup