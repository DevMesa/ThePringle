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

	static void GetOBB(uint32_t index, Objects::ObjectBase* data, Pringle::Vector& mins, Pringle::Vector& maxs)
	{
		auto bb = Forge::GetObjectBoundingBox(data->TagIndex);
		if (!bb)
			return;

		Math::RealMatrix4x3 objectTransform;
		GetObjectTransformationMatrix(index, &objectTransform);
		const auto objectRotation = Math::RealQuaternion::CreateFromRotationMatrix(objectTransform);

		Vector pos = data->Position;// Math::RealVector3D::Transform(data->Center, objectRotation);

		mins.X = pos.X + bb->MinX;
		mins.Y = pos.Y + bb->MinY;
		mins.Z = pos.Z + bb->MinZ;

		maxs.X = pos.X + bb->MaxX;
		maxs.Y = pos.Y + bb->MaxY;
		maxs.Z = pos.Z + bb->MaxZ;
	}
}

namespace Pringle
{
	static bool isMainMenu = true;

	ESP::ESP() : ModuleBase("Pringle")
	{
		Enabled = this->AddVariableInt("ESP.Enabled", "esp.enabled", "Enables ESP", eCommandFlagsArchived, 0);

		Hook::SubscribeMember<DirectX::EndScene>(this, &ESP::OnEndScene);
		//Hook::SubscribeMember<Hooks::PreTick>(this, &ESP::OnPreTick);

		Patches::Core::OnMapLoaded([](auto map) {
			isMainMenu = !(std::string(map).find("mainmenu") == std::string::npos);
		});
	}

	void ESP::Draw(const DirectX::EndScene& msg, uint32_t index, Blam::Objects::ObjectBase* unit, uint32_t color)
	{
		Vector mins, maxs;
		GetOBB(index, unit, mins, maxs);

		int topX, topY;
		if (!msg.Draw.ToScreen(maxs.X, maxs.Y, maxs.Z, topX, topY))
			return;

		int botX, botY;
		if (!msg.Draw.ToScreen(mins.X, mins.Y, mins.Z, botX, botY))
			return;

		int height = (botY - topY);
		int width = height;

		msg.Draw.OutlinedRect(topX, topY, width, height, color);
	}

	void ESP::Draw(const DirectX::EndScene& msg, Blam::Math::RealVector3D _pos, uint32_t color)
	{

	}

	void ESP::DrawPlayers(const DirectX::EndScene & msg)
	{
		
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

		//std::lock_guard<std::mutex> lock(this->unit_mutex);

		msg.Draw.CaptureState();

		//this->DrawPlayers(msg);
		this->DrawObjects(msg);

		msg.Draw.ReleaseState();
	}

	void ESP::UpdatePlayers(const Hooks::PreTick& msg)
	{
		ally_player_units.clear();
		enemy_player_units.clear();
		hit_enemy_player_units.clear();

		auto& players = Players::GetPlayers();
		auto& localPlayerIndex = Players::GetLocalPlayer(0);
		auto localPlayer = players.Get(localPlayerIndex);
		if (!localPlayer)
			return;

		auto localPlayerUnit = Objects::Get(localPlayer->SlaveUnit);
		if (!localPlayerUnit)
			return;

		for (auto it = players.begin(); it != players.end(); ++it) {
			const auto& unitObjectIndex = it->SlaveUnit.Handle;
			if (unitObjectIndex == -1)
				continue;

			if (localPlayer->SlaveUnit.Handle == unitObjectIndex)
				continue;

			const auto& unit = Objects::Get(unitObjectIndex);
			if (!unit)
				continue;

			if (it->DeadSlaveUnit)
				continue;

			if (it->Properties.TeamIndex != localPlayer->Properties.TeamIndex)
			{
				Vector playerPos;
				Vector targetPos;

				Unit_GetHeadPosition(localPlayer->SlaveUnit, &playerPos);
				Unit_GetHeadPosition(unitObjectIndex, &targetPos);

				if (Halo::SimpleHitTest(playerPos, targetPos, localPlayer->SlaveUnit, -1)) {
	
					hit_enemy_player_units.push_back(unit);
				}
				else
					enemy_player_units.push_back(unit);
			}
			else
				ally_player_units.push_back(unit);
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
			this->UpdateObjects(msg, localPlayer, localPlayerUnit);
		}
	}
}
