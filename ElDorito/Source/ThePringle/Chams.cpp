#include "Chams.hpp"
#include "../Blam/BlamObjects.hpp"
#include "../Blam/BlamPlayers.hpp"

#include <d3d9.h>
#include <d3dx9core.h>

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

		inline bool HasDatum()
		{
			return Datum != nullptr;
		}

		inline bool HasHeader()
		{
			return Header != nullptr;
		}

		inline bool IsEmpty()
		{
			return !HasDatum() && !HasHeader();
		}

		bool IsAlive()
		{
			return !IsEmpty()
				&& Datum->SlaveUnit != DatumHandle::Null
				&& Header->GetSalt() == Objects::GetObjects().Get(Datum->SlaveUnit)->GetSalt();
		}

		bool IsDead()
		{
			return !IsEmpty()
				&& Datum->DeadSlaveUnit != DatumHandle::Null
				&& Header->GetSalt() == Objects::GetObjects().Get(Datum->DeadSlaveUnit)->GetSalt();
		}

		PlayerInfo() : Datum(nullptr), Header(nullptr) { };
		PlayerInfo(Objects::ObjectHeader* header) : Datum(nullptr), Header(header)
		{
			if (header->Data)
			{
				auto index = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(header->Data) + 0x198);
				if (index != -1)
				{
					auto pl = Players::GetPlayers().Get(index);
					if (pl)
						this->Datum = pl;
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

	static HRESULT CreateShader(LPDIRECT3DDEVICE9 device, LPDIRECT3DPIXELSHADER9* shader)
	{
		static const char PS_COLOR_DEPTH[] = "ps_3_0\nmov oC0, c199\nmov oDepth, c223.r"; // mostly unused registers

		LPD3DXBUFFER buffer = NULL;

		auto err = S_OK;
		if (FAILED(err = D3DXAssembleShader(PS_COLOR_DEPTH, sizeof(PS_COLOR_DEPTH), NULL, NULL, NULL, &buffer, NULL)))
			return err;

		if (FAILED(err = device->CreatePixelShader(reinterpret_cast<DWORD*>(buffer->GetBufferPointer()), shader)))
			return err;

		buffer->Release();

		return S_OK;
	}

	static LPDIRECT3DPIXELSHADER9 PS_Chams;
}

namespace Pringle
{
	static const float CHAMS_COLOR_ALLY[]		= { 0.f, 1.f, 0.f, 1.f };
	static const float CHAMS_COLOR_ENEMY[]		= { 1.f, 0.f, 0.f, 1.f };
	static const float CHAMS_COLOR_DEAD[]		= { 1.f, 1.f, 1.f, 1.f };

	std::unique_ptr<PlayerInfo> LocalPlayer_ptr = nullptr;
	std::unique_ptr<PlayerInfo> Player_ptr = nullptr;

	Chams::Chams() : Modules::ModuleBase("pringle")
	{
		Enabled = this->AddVariableInt("chams.enabled", "chams.enabled", "Enable chams", eCommandFlagsArchived, 1);

		Hook::SubscribeMember<Hooks::DirectX::Initialize>(this, &Chams::OnDirectXInitialize);
		Hook::SubscribeMember<Hooks::DirectX::BeginScene>(this, &Chams::OnBeginScene);
		Hook::SubscribeMember<Hooks::DirectX::DrawObjectPrimitive>(this, &Chams::OnDrawObjectPrimitive);
	}

	void Chams::OnDirectXInitialize(const Hooks::DirectX::Initialize& msg)
	{
		CreateShader(msg.Device, &PS_Chams);
	}

	void Chams::OnBeginScene(const Hooks::DirectX::BeginScene& msg)
	{
		LocalPlayer_ptr = std::make_unique<PlayerInfo>(GetLocalPlayer(true));
		Player_ptr = nullptr;
	}

	void Chams::OnDrawObjectPrimitive(const Hooks::DirectX::DrawObjectPrimitive& msg)
	{
		if (Enabled->ValueInt == 0)
			return;

		auto object = msg.Target;

		switch (object->Type)
		{
		case Objects::eObjectTypeScenery:
			object = Blam::Objects::GetObjects().Get(object->Data->Parent); // this for the extra armor rendered on the player, apparently its scenery???
			if (!object || object->Type != Objects::eObjectTypeBiped)
				return;
		case Objects::eObjectTypeBiped:
			break;
		default:
			return;
		}

		if (!Player_ptr 
			|| Player_ptr->IsEmpty()
			|| object->GetSalt() != Player_ptr->Header->GetSalt())
			Player_ptr = std::make_unique<PlayerInfo>(object); // cache to prevent multiple lookups

		if (Player_ptr->IsEmpty())
		{
			Player_ptr = nullptr;
			return;
		}

		auto& device = msg.Device;

		if (LocalPlayer_ptr && !LocalPlayer_ptr->IsEmpty())
		{
			if (LocalPlayer_ptr->HasDatum() && Player_ptr->HasDatum())
			{
				if (LocalPlayer_ptr->Datum->GetSalt() == Player_ptr->Datum->GetSalt())
					return; // don't render on self
				if (LocalPlayer_ptr->Datum->Properties.TeamIndex == Player_ptr->Datum->Properties.TeamIndex)
					device->SetPixelShaderConstantF(199, CHAMS_COLOR_ALLY, 1);
				else
					device->SetPixelShaderConstantF(199, CHAMS_COLOR_ENEMY, 1);
			}
			else if (LocalPlayer_ptr->HasHeader() && Player_ptr->HasHeader() && LocalPlayer_ptr->Header->GetSalt() == Player_ptr->Header->GetSalt())
				return; // don't render on self
			else
				device->SetPixelShaderConstantF(199, CHAMS_COLOR_DEAD, 1);
		}
		else
		{
			device->SetPixelShaderConstantF(199, CHAMS_COLOR_DEAD, 1);
		}

		static const float DEPTH[4] = { 0.f, 0.f, 0.f, 0.f }; // depth[0] stores the depth value
		device->SetPixelShaderConstantF(223, DEPTH, 1);

		device->SetPixelShader(PS_Chams);
	}
}