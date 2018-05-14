#include "ESP.hpp"
#include "Draw.hpp"
#include "Halo/Halo.hpp"
#include "../ElDorito.hpp"
#include "../Pointer.hpp"
#include "../Patches/Core.hpp"
#include "../Blam/BlamData.hpp"
#include "../Blam/BlamPlayers.hpp"
#include "../Blam/BlamObjects.hpp"
#include "../Blam/Math/RealVector3D.hpp"
#include "../Forge/ForgeUtil.hpp"
#include "../Console.hpp"
#include "../Modules/ModuleSettings.hpp"
#include "../Forge/ForgeUtil.hpp"
#include "../Blam/Math/RealMatrix4x3.hpp"
#include "../Blam/Math/RealQuaternion.hpp"
#include "FontManager.hpp"
#include "../Patches/Weapon.hpp"

#include "Vector.hpp"
#include "QAngle.hpp"

#include <d3dx9math.h>
#include <d3dx9core.h>

#include <fstream>
#include <detours.h>
#include <mutex>

using namespace Pringle;
using namespace Pringle::Hooks;
using namespace Modules;
using namespace Blam;
using namespace Patches;

namespace
{
	const static auto Unit_GetHeadPosition = (void(__cdecl*)(uint32_t unitObjectIndex, Vector* position))(0x00B439D0);
	const auto GetObjectTransformationMatrix = (void(__cdecl*)(uint32_t objectIndex, Blam::Math::RealMatrix4x3* outMatrix))(0x00B2EC60);

	static Players::PlayerDatum* GetLocalPlayer()
	{
		auto& handle = Players::GetLocalPlayer(0);
		if (handle == DatumHandle::Null)
			return NULL;

		auto pl = Players::GetPlayers().Get(handle);
		if (!pl)
			return NULL;

		return pl;
	}

	static Objects::ObjectBase* GetLocalPlayerUnit()
	{
		auto lp = GetLocalPlayer();
		if (!lp)
			return NULL;

		auto unit = Objects::Get(lp->SlaveUnit);
		if (!unit)
			return NULL;

		return unit;
	}

	static void GetHeadPos(uint32_t unitObjectIndex, const Vector& shootDir, Vector& position)
	{
		Unit_GetHeadPosition(unitObjectIndex, &position);
		position += shootDir * 0.05f;
		position += shootDir.Right() * 0.05f;
		position += Vector::Down() * 0.01f;
	}

	static bool GetEquippedWeapon(Patches::Weapon::WeaponInfo& info, Objects::ObjectBase* unit)
	{
		auto ptr = reinterpret_cast<uint8_t*>(unit);
		if (!ptr)
			return false;

		auto weaponIndex = reinterpret_cast<uint8_t*>(ptr + 0x2CA);
		if (weaponIndex[0] == 0xFF)
			return false;

		auto weapons = reinterpret_cast<uint32_t*>(ptr + 0x2D0);
		auto weapon = Objects::Get(weapons[weaponIndex[0]]);
		if (!weapon)
			return false;

		info.Index = weapon->TagIndex;
		info.Name = Patches::Weapon::GetName(info);
		info.TagName = Patches::Weapon::GetTagName(info);
		info.Offset = Patches::Weapon::GetOffsets(info);

		return true;
	}
}

namespace Pringle
{
	static bool isMainMenu = true;

	ESP::ESP() : ModuleBase("Pringle")
	{
		Enabled = this->AddVariableInt("ESP.Enabled", "esp.enabled", "Enables ESP", eCommandFlagsArchived, 0);

		Hook::SubscribeMember<DirectX::EndScene>(this, &ESP::OnEndScene);
		Hook::SubscribeMember<Hooks::PreTick>(this, &ESP::OnPreTick);

		Patches::Core::OnMapLoaded([](auto map) {
			isMainMenu = !(std::string(map).find("mainmenu") == std::string::npos);
		});
	}

	void ESP::Draw(const DirectX::EndScene& msg, uint32_t index, Blam::Objects::ObjectBase* unit, uint32_t color)
	{

	}

	void ESP::Draw(const DirectX::EndScene& msg, Blam::Math::RealVector3D _pos, uint32_t color)
	{

	}

	void ESP::DrawPlayers(const DirectX::EndScene & msg)
	{
		auto& players = Players::GetPlayers();
		auto& localPlayerIndex = Players::GetLocalPlayer(0);
		auto localPlayer = players.Get(localPlayerIndex);
		if (!localPlayer)
			return;

		auto teamIndex = localPlayer->Properties.TeamIndex;

		auto localPlayerUnit = Objects::Get(localPlayer->SlaveUnit);
		if (!localPlayerUnit)
			return;

		for (auto it = this->players.begin(); it != this->players.end(); ++it)
		{
			auto info = *it;

			auto unit = info.Data;

			uint32_t color;
			if (info.Player.Properties.TeamIndex == teamIndex)
				color = info.Visible ? COLOR4I(0, 255, 0, 255) : COLOR4I(0, 0, 255, 255);
			else
				color = info.Visible ? COLOR4I(255, 0, 0, 255) : COLOR4I(255, 128, 40, 255);

			Vector pbot = unit->Position;
			Vector ptop = pbot;
			ptop.Z += info.Maxs.Z;

			int topX, topY, botX, botY;

			bool vis1 = msg.Draw->ToScreen(ptop.X, ptop.Y, ptop.Z, topX, topY);
			bool vis2 = msg.Draw->ToScreen(pbot.X, pbot.Y, pbot.Z, botX, botY);
			// do not move into the if statement, it will leave botX and botY unset if the first condition is true 
			if (!vis1 && !vis2)
				continue;

			int height = (botY - topY);
			int width = height;

			msg.Draw->OutlinedRect(topX - (width / 2), topY, width, height, color);

			const auto name = info.Player.Properties.DisplayName;

			int twidth, theight;
			FontManager::Instance().GetTextDimensions(name, twidth, theight);

			int drawY = topY - theight;

			Weapon::WeaponInfo winfo;
			if (GetEquippedWeapon(winfo, unit))
			{
				auto wname = winfo.Name.c_str();

				// draw text with shadow
				msg.Draw->Text(wname, topX + 1, drawY, COLOR4I(0, 0, 0, 255), DT_CENTER);
				msg.Draw->Text(wname, topX, drawY - 1, COLOR4I(255, 255, 255, 255), DT_CENTER);

				drawY -= theight;
			}

			// draw text with shadow
			msg.Draw->Text(name, topX + 1, drawY, COLOR4I(0, 0, 0, 255), DT_CENTER);
			msg.Draw->Text(name, topX, drawY - 1, COLOR4I(255, 255, 255, 255), DT_CENTER);
		}
	}

	void ESP::DrawObjects(const DirectX::EndScene& msg)
	{
		/*
		auto& players = Players::GetPlayers();
		auto& localPlayerIndex = Players::GetLocalPlayer(0);
		auto localPlayer = players.Get(localPlayerIndex);
		if (!localPlayer)
			return;

		auto localPlayerUnit = Objects::Get(localPlayer->SlaveUnit);
		if (!localPlayerUnit)
			return;*/

		auto& objects = Blam::Objects::GetObjects();
		for (auto it = objects.begin(); it != objects.end(); ++it)
		{
			auto header = *it;
			if (header.IsNull())
				continue;

			if (header.Type != Objects::ObjectType::eObjectTypeBiped 
				&& header.Type != Objects::ObjectType::eObjectTypeVehicle 
				&& header.Type != Objects::ObjectType::eObjectTypeWeapon 
				&& header.Type != Objects::ObjectType::eObjectTypeEquipment
				&& header.Type != Objects::ObjectType::eObjectTypeProjectile)
				continue;

			auto unit = header.Data;

			if (!unit)
				continue;

			uint32_t color = D3DCOLOR_RGBA(0, 255, 0, 150);

			this->Draw(msg, it.CurrentDatumIndex.Handle, unit, color);
		}

		/*
		for (auto it = hit_objects.begin(); it != hit_objects.end(); ++it)
		{
			auto unit = *it;
			uint32_t color = D3DCOLOR_RGBA(255, 255, 255, 255);

			if (!unit)
				continue;

			this->Draw(msg, unit, color);
		}

		for (auto it = hit_points.begin(); it != hit_points.end(); ++it)
		{
			auto point = it->end;
			uint32_t color = D3DCOLOR_RGBA(0, 0, 255, 255);

			this->Draw(msg, point, color);
		}
		*/
	}

	void ESP::OnEndScene(const DirectX::EndScene & msg)
	{
		if (isMainMenu || this->Enabled->ValueInt == 0)
			return;

		auto localPlayer = GetLocalPlayer();
		if (!localPlayer)
			return;

		auto localPlayerUnit = GetLocalPlayerUnit();
		if (!localPlayerUnit)
			return;

		std::lock_guard<std::mutex> lock(this->unit_mutex);

		msg.Draw->CaptureState();

		this->DrawPlayers(msg);
		//this->DrawObjects(msg);

		msg.Draw->ReleaseState();
	}

	void ESP::UpdatePlayers(const Hooks::PreTick& msg)
	{
		players.clear();

		auto& players = Players::GetPlayers();
		auto& localPlayerIndex = Players::GetLocalPlayer(0);
		auto localPlayer = players.Get(localPlayerIndex);
		if (!localPlayer)
			return;

		auto localPlayerUnit = Objects::Get(localPlayer->SlaveUnit);
		if (!localPlayerUnit)
			return;

		Vector playerPos;
		GetHeadPos(localPlayer->SlaveUnit, localPlayerUnit->Forward, playerPos);

		for (auto it = players.begin(); it != players.end(); ++it) {
			auto& player = *it;
			auto& unitObjectIndex = it->SlaveUnit.Handle;
			if (unitObjectIndex == -1)
				continue;

			if (localPlayer->SlaveUnit.Handle == unitObjectIndex)
				continue;

			auto unit = Objects::Get(unitObjectIndex);
			if (!unit)
				continue;

			if (it->DeadSlaveUnit)
				continue;

			Vector targetPos;
			GetHeadPos(unitObjectIndex, unit->Forward, targetPos);

			bool visible = Halo::SimpleHitTest(playerPos, targetPos, localPlayer->SlaveUnit, unitObjectIndex);

			Vector mins(0, 0, 0), maxs(0, 0, 0); // we don't watch uninitialized vectors
			auto bb = Forge::GetObjectBoundingBox(unit->TagIndex);
			if (bb)
			{
				mins = Vector(bb->MinX, bb->MinY, bb->MinZ);
				maxs = Vector(bb->MaxX, bb->MaxY, bb->MaxZ);
			}

			this->players.emplace_back(player, unit, visible, mins, maxs);
		}
	}

	void ESP::UpdateObjects(const Hooks::PreTick& msg, Blam::Players::PlayerDatum* localPlayer, Blam::Objects::ObjectBase* localPlayerUnit)
	{
		objects.clear();
		hit_objects.clear();
		hit_points.clear();

		auto& objects = Blam::Objects::GetObjects();
		for (auto it = objects.begin(); it != objects.end(); ++it) {
			const auto& unitObjectHeader = *it;
			if (unitObjectHeader.IsNull())
				continue;

			if (localPlayer->SlaveUnit == unitObjectHeader.GetTagHandle())
				continue;

			const auto& unit = unitObjectHeader.Data;
			if (!unit)
				continue;

			{
				this->objects.push_back(unit);
			}		
		}
	}

	void ESP::OnPreTick(const Hooks::PreTick & msg)
	{
		auto& players = Players::GetPlayers();
		auto& localPlayerIndex = Players::GetLocalPlayer(0);
		auto localPlayer = players.Get(localPlayerIndex);
		if (!localPlayer)
			return;

		auto localPlayerUnit = Objects::Get(localPlayer->SlaveUnit);
		if (!localPlayerUnit)
			return;

		{
			std::lock_guard<std::mutex> lock(unit_mutex);

			this->UpdatePlayers(msg);
			//this->UpdateObjects(msg, localPlayer, localPlayerUnit);
		}
	}
}
