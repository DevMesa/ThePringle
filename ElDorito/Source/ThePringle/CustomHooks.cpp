#include <stdio.h>  
#include <intrin.h> 

#pragma intrinsic(_ReturnAddress)

#include "CustomHooks.hpp"
#include "Hooks.hpp"
#include "Chams.hpp"

#include <detours.h>

#define D3D_DRAWPRIMITIVE_INDEX 82

namespace Pringle::CustomHooks
{
	typedef long(__stdcall* DrawIndexedPrimitive_t)(void*, int, int, unsigned int, unsigned int, unsigned int, unsigned int);

	DrawIndexedPrimitive_t DrawIndexedPrimitive_ptr = 0;

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

	long __stdcall DrawIndexedPrimitive_Hook(LPDIRECT3DDEVICE9 thisptr, int type, int BaseVertexIndex, unsigned int MinVertexIndex, unsigned int NumVertices, unsigned int startIndex, unsigned int primCount)
	{
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
		} 
		else 
			return DrawIndexedPrimitive_ptr(thisptr, type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
	}

	void DirectX::Initialize(LPDIRECT3DDEVICE9 device)
	{
		MakeTexture(device, texture_red, D3DCOLOR_RGBA(255, 0, 0, 255));

		auto vtable = reinterpret_cast<uint32_t*>(*reinterpret_cast<uint32_t*>(device));
		DrawIndexedPrimitive_ptr = reinterpret_cast<DrawIndexedPrimitive_t>(vtable[D3D_DRAWPRIMITIVE_INDEX]);
		
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(reinterpret_cast<void**>(&DrawIndexedPrimitive_ptr), &DrawIndexedPrimitive_Hook);
		DetourTransactionCommit();
	}
}

class D3DInitializer
{
public:
	D3DInitializer()
	{
		Pringle::Hook::SubscribeMember<Pringle::Hooks::DirectX::Initialize>(this, &D3DInitializer::OnInitialize);
	}

	void OnInitialize(const Pringle::Hooks::DirectX::Initialize& msg)
	{
		Pringle::CustomHooks::DirectX::Initialize(msg.Device);
	}
};

D3DInitializer _;