#include "Aimbot.hpp"
#include "Halo/Halo.hpp"

#include <string>  

#include <iostream>
#include <algorithm>
#include <cmath>

#include "../ElDorito.hpp"
#include "../Blam/BlamTypes.hpp"
#include "../Blam/BlamPlayers.hpp"
#include "../Blam/BlamObjects.hpp"
#include "../Blam/Math/RealVector3D.hpp"
#include "../Console.hpp"

using namespace Pringle;
using namespace Pringle::Hooks;
using namespace Modules;

using Vector = Blam::Math::RealVector3D;

const static auto Unit_GetHeadPosition = (void(__cdecl*)(uint32_t unitObjectIndex, Vector *position))(0x00B439D0);

void Aimbot::Initalize()
{
	static Aimbot AimbotInstance;
}

Aimbot::Aimbot() : ModuleBase("pringle")
{
	this->Enabled = this->AddVariableInt("aimbot.enabled", "aimbot.enabled", "Enable the hack", eCommandFlagsArchived, 0);
	this->X = this->AddVariableInt("aimbot.x", "aimbot.x", "Enable the hack", eCommandFlagsArchived, 0);
	this->Y = this->AddVariableFloat("aimbot.y", "aimbot.y", "Enable the hack", eCommandFlagsArchived, 0);

	Hook::SubscribeMember<PostTick>(this, &Aimbot::OnTick);
	Hook::SubscribeMember<GetTargets>(this, &Aimbot::OnGetTargets);
}

void Aimbot::OnGetTargets(const GetTargets& msg)
{
	auto& players = Blam::Players::GetPlayers();
	auto& localplayerhandle = Blam::Players::GetLocalPlayer(0);

	for (auto it = players.begin(); it != players.end(); ++it) 
	{
		const auto& unitObjectIndex = it->SlaveUnit.Handle;
		
		if (unitObjectIndex == -1)
			continue;

		if (players.Get(localplayerhandle)->SlaveUnit.Handle == unitObjectIndex)
			continue;

		const auto& unit = Blam::Objects::Get(unitObjectIndex);
		
		if (!unit)
			continue;

		if (it->DeadSlaveUnit)
			continue;

		if (it->Properties.TeamIndex == players.Get(localplayerhandle)->Properties.TeamIndex)
			continue;

		auto localPlayerUnitIndex = players.Get(localplayerhandle)->SlaveUnit;

		Vector playerPos;
		Vector targetPos;

		Unit_GetHeadPosition(localPlayerUnitIndex, &playerPos);
		Unit_GetHeadPosition(unitObjectIndex, &targetPos);

		if (!Halo::SimpleHitTest(&playerPos, &targetPos, localPlayerUnitIndex, -1))
			continue;

		msg.Targets.emplace_back(unit->Position, unit->Velocity);
	}

}

static double Scale(double x, double from_min, double from_max, double to_min, double to_max)
{
	double from_diff = from_max - from_min;
	double to_diff = to_max - to_min;

	double y = (x - from_min) / from_diff;
	return to_min + to_diff * y;
}

void Aimbot::OnTick(const PostTick& msg)
{
	const auto& localplayerphandle = Blam::Players::GetLocalPlayer(0);
	const auto& localplayer = Blam::Players::GetPlayers().Get(localplayerphandle);

	if (localplayer == nullptr)
		return;

	const auto& localplayerunit = Blam::Objects::Get(localplayer->SlaveUnit.Handle);

	if (localplayerunit == nullptr)
		return;

	Pointer inputptr = ElDorito::GetMainTls(GameGlobals::Input::TLSOffset)[0];

	Pointer &pitchptr = inputptr(GameGlobals::Input::VerticalViewAngle);
	Pointer &yawptr = inputptr(GameGlobals::Input::HorizontalViewAngle);

	float pitch = pitchptr.Read<float>();
	float yaw = yawptr.Read<float>();
	
	const auto& localpos = localplayerunit->Position;

	if (this->Enabled->ValueInt != 0)
	{
		CachedTargets.clear();
		Hook::Call<Hooks::GetTargets>(CachedTargets);

		if (CachedTargets.size() == 0)
			return;
		
		auto& besttarget = CachedTargets.back();
		float bestdist = (besttarget.Position - localpos).Length();

		for (const auto& target : CachedTargets)
		{
			const auto& targetpos = target.Position;

			float dist = (targetpos - localpos).Length();

			if (dist < bestdist)
			{
				bestdist = dist;
				besttarget = target;
			}

		}

		Vector dir = localpos - besttarget.Position;
		float len = std::sqrt(dir.I*dir.I + dir.J*dir.J + dir.K*dir.K);

		dir /= len;

		yaw = std::atan2(dir.J, dir.I);
		pitch = -std::asin(dir.K);

		yaw += 3.14159f;
	
		pitchptr.WriteFast(pitch);
		yawptr.WriteFast(yaw);
	}
}

