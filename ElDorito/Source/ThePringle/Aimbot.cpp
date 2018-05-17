#include "Aimbot.hpp"
#include "Util.hpp"
#include "Halo/Halo.hpp"

#include <string>  

#include <iostream>
#include <algorithm>
#include <cmath>
#include <limits>

#include "../ElDorito.hpp"
#include "../Blam/BlamTypes.hpp"
#include "../Blam/BlamPlayers.hpp"
#include "../Blam/BlamObjects.hpp"
#include "../Blam/BlamInput.hpp"
#include "../Console.hpp"

using namespace Pringle;
using namespace Pringle::Hooks;
using namespace Modules;

const static auto Unit_GetHeadPosition = (void(__cdecl*)(uint32_t unitObjectIndex, Vector* position))(0x00B439D0);

Aimbot::Aimbot() : ModuleBase("Pringle")
{
	this->Enabled = this->AddVariableInt("Aimbot.Enabled", "aimbot.enabled", "Enable the hack", eCommandFlagsArchived, 0);
	this->AutoShoot = this->AddVariableInt("Aimbot.AutoShoot", "aimbot.autoshoot", "Enable autoshooting", eCommandFlagsArchived, 1);
	this->X = this->AddVariableInt("Aimbot.X", "aimbot.x", "Enable the hack", eCommandFlagsArchived, 0);
	this->Y = this->AddVariableFloat("Aimbot.Y", "aimbot.y", "Enable the hack", eCommandFlagsArchived, 0);
	
	this->AimPos = this->AddVariableString("Aimbot.AimPos", "aimbot.aimpos", "head, center, origin", eCommandFlagsArchived, "center");

	this->DistanceHalfAt = this->AddVariableFloat("Aimbot.Importance.Distance.HalfAt", "aimbot.importance.distance.halfat", "0.5 importance at x distance", eCommandFlagsArchived, 20.0f);
	this->DistanceImportance = this->AddVariableFloat("Aimbot.Importance.Distance", "aimbot.importance.distance", "Distance importance", eCommandFlagsArchived, 0.5f);

	this->CenterImportance = this->AddVariableFloat("Aimbot.Importance.Center", "aimbot.importance.center", "Importance of them being in the center of the screen", eCommandFlagsArchived, 0.5f);
	this->AliveImportance = this->AddVariableFloat("Aimbot.Importance.Alive", "aimbot.importance.alive", "Importance of them being alive", eCommandFlagsArchived, 1.0f);
	this->TeamImportance = this->AddVariableFloat("Aimbot.Importance.Team", "aimbot.importance.team", "Importance of them being on a different team", eCommandFlagsArchived, 1.0f);
	this->VisibleImportance = this->AddVariableFloat("Aimbot.Importance.Visible", "aimbot.importance.visible", "Importance of them being visible.", eCommandFlagsArchived, 1.0f);

	Hook::SubscribeMember<PostTick>(this, &Aimbot::OnTick);
	Hook::SubscribeMember<PreLocalPlayerInput>(this, &Aimbot::OnPreLocalPlayerInput);
	Hook::SubscribeMember<AimbotEvents::GetTargets>(this, &Aimbot::GetPlayers);
	Hook::SubscribeMember<AimbotEvents::ScoreTarget>(this, &Aimbot::ScoreDistance);
	Hook::SubscribeMember<AimbotEvents::ScoreTarget>(this, &Aimbot::ScoreCenter);
	Hook::SubscribeMember<AimbotEvents::ScoreTarget>(this, &Aimbot::ScoreAlive);
	Hook::SubscribeMember<AimbotEvents::ScoreTarget>(this, &Aimbot::ScoreTeam);
	Hook::SubscribeMember<AimbotEvents::ScoreTarget>(this, &Aimbot::ScoreVisible);
}

void Aimbot::GetPlayers(const AimbotEvents::GetTargets& msg)
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

		auto unit = Blam::Objects::Get(unitObjectIndex);
		
		if (!unit)
			continue;

		Vector pos;
		Vector& vel = reinterpret_cast<Vector&>(unit->Velocity);
		Vector& shoot_dir = reinterpret_cast<Vector&>(unit->Forward);

		switch (msg.AimAt)
		{
		default:
		case AimbotEvents::AimPosition::Origin:
			pos = unit->Position;
			break;
		case AimbotEvents::AimPosition::Head:
			Unit_GetHeadPosition(unitObjectIndex, &pos);
			pos += shoot_dir * 0.05f; // forward ness for the head pos
			pos += shoot_dir.Right() * 0.05f;
			pos += Vector::Down() * 0.01f;
			break;
		case AimbotEvents::AimPosition::Center:
			pos = unit->Center;
			break;
		}

		QAngle null_viewang = QAngle(Quaternion::Identity());
		AimbotEvents::Target::Info info(
			/*Velocity =*/ vel,
			/*ShootDirection =*/ shoot_dir,
			/*ViewAngles =*/ null_viewang,
			/*OriginOffset =*/ pos - unit->Position,
			/*Team =*/ it->Properties.TeamIndex,
			/*UnitIndex =*/ unitObjectIndex,
			/*Alive =*/ !it->DeadSlaveUnit,
			/*Player =*/ true
		);

		msg.GotTarget(AimbotEvents::Target(pos, info));
	}

}

void Pringle::Aimbot::ScoreDistance(const AimbotEvents::ScoreTarget& msg)
{
	// higher = more linear, distance is more important farther away
	// lowering it means distance is less important farther away
	// the score will be * 0.5f at x units away
	float point5_at_distance = this->DistanceHalfAt->ValueFloat;
	float distance_importance = this->DistanceImportance->ValueFloat;

	float distance = (msg.What.Position - msg.Self.Position).Length();
	msg.Importance *= 
		AimbotEvents::ScoreTarget::Calculate(
			point5_at_distance / (point5_at_distance + distance),
			distance_importance);
}

void Pringle::Aimbot::ScoreCenter(const AimbotEvents::ScoreTarget& msg)
{
	float dot = msg.Self.Information.ShootDirection
		.Dot(msg.What.Position - msg.Self.Position);
	float perc = (dot + 1.0f) * 0.5f;

	msg.Importance *= 
		AimbotEvents::ScoreTarget::Calculate(
			perc,
			this->CenterImportance->ValueFloat);
}

void Pringle::Aimbot::ScoreAlive(const AimbotEvents::ScoreTarget& msg)
{
	msg.Importance *=
		AimbotEvents::ScoreTarget::Calculate(
			msg.What.Information.Alive ? 1.0f : 0.0f,
			this->AliveImportance->ValueFloat);
}

void Pringle::Aimbot::ScoreTeam(const AimbotEvents::ScoreTarget& msg)
{
	msg.Importance *=
		AimbotEvents::ScoreTarget::Calculate(
			msg.What.Information.Team == msg.Self.Information.Team ? 0.0f : 1.0f,
			this->TeamImportance->ValueFloat);
}

void Pringle::Aimbot::ScoreVisible(const AimbotEvents::ScoreTarget& msg)
{
	//bool visible = Halo::SimpleHitTest(&playerPos, &targetPos, localPlayerUnitIndex, -1));
	bool visible = Halo::SimpleHitTest(msg.Self.Position, msg.What.Position, msg.Self.Information.UnitIndex, msg.What.Information.UnitIndex);

	msg.Importance *=
		AimbotEvents::ScoreTarget::Calculate(
			visible ? 1.0f : 0.0f,
			this->VisibleImportance->ValueFloat);
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
	this->Shoot = false;
	if (this->Enabled->ValueInt == 0)
	{
		this->LastSelfPositionFresh = false;
		this->LastTargetUnitIndex = 0;
		return;
	}

	const auto& localplayerphandle = Blam::Players::GetLocalPlayer(0);
	const auto& localplayer = Blam::Players::GetPlayers().Get(localplayerphandle);

	if (localplayer == nullptr)
		return;

	auto localplayerunit = Blam::Objects::Get(localplayer->SlaveUnit.Handle);

	if (localplayerunit == nullptr)
		return;

	Pointer inputptr = ElDorito::GetMainTls(GameGlobals::Input::TLSOffset)[0];

	Pointer& pitchptr = inputptr(GameGlobals::Input::VerticalViewAngle);
	Pointer& yawptr = inputptr(GameGlobals::Input::HorizontalViewAngle);

	float pitch = pitchptr.Read<float>();
	float yaw = yawptr.Read<float>();

	Vector shootpos;
	Unit_GetHeadPosition(localplayer->SlaveUnit, &shootpos);

	{ // delta for shootpos
		if (this->LastSelfPositionFresh)
		{
			Vector copy = shootpos;
			shootpos += (shootpos - this->LastSelfPosition);
			this->LastSelfPosition = copy;
		}
		else
		{
			this->LastSelfPosition = shootpos;
			this->LastSelfPositionFresh = true;
		}
	}

	Vector shootdir = Vector(Angle::Radians(pitch), Angle::Radians(yaw)); // TODO: localplayerunit->Forward
	
	QAngle null_viewang = QAngle(Quaternion::Identity());
	AimbotEvents::Target::Info selfinfo(
		/*Velocity =*/ reinterpret_cast<Vector&>(localplayerunit->Velocity),
		/*ShootDirection =*/ shootdir,
		/*ViewAngles =*/ null_viewang,
		/*OriginOffset =*/ shootpos - localplayerunit->Position,
		/*Team =*/ localplayer->Properties.TeamIndex,
		/*UnitIndex =*/ localplayer->SlaveUnit.Handle,
		/*Alive =*/ !localplayer->DeadSlaveUnit,
		/*Player =*/ true
	);

	AimbotEvents::Target self(shootpos, selfinfo);

	{
		Vector best_pos;
		int best_unitindex = 0;
		float best_score = std::numeric_limits<float>::epsilon(); // the lowest score we will accept
		bool best_disableautoshoot = false;
		bool best_got = false;

		AimbotEvents::AimPosition::Enum aimpos = AimbotEvents::AimPosition::Origin;

		if (this->AimPos->ValueString == "head")
			aimpos = AimbotEvents::AimPosition::Head;
		else if (this->AimPos->ValueString == "center")
			aimpos = AimbotEvents::AimPosition::Center;
		else if (this->AimPos->ValueString == "origin")
			aimpos = AimbotEvents::AimPosition::Origin;

		Hook::Call<AimbotEvents::GetTargets>([&](const AimbotEvents::Target& targ) -> void
		{
			AimbotEvents::ScoreTarget e(self, targ);
			Hook::CallPremade<AimbotEvents::ScoreTarget>(e);

			if (e.Importance > best_score)
			{
				best_score = e.Importance;
				best_pos = targ.Position;
				best_unitindex = targ.Information.UnitIndex;
				best_disableautoshoot = e.DisableAutoshoot;
				best_got = true;
			}
		}, aimpos);

		if (!best_got)
		{
			this->LastTargetUnitIndex = 0;
			return;
		}
		
		// delta positions
		{
			if (best_unitindex == this->LastTargetUnitIndex)
			{
				Vector copy = best_pos;
				best_pos += (best_pos - this->LastTargetPosition);
				this->LastTargetPosition = copy;

				// shoot is here so we use the non-laggy position for shooting
				this->Shoot = this->AutoShoot->ValueInt != 0 && !best_disableautoshoot;
			}
			else
			{
				this->LastTargetUnitIndex = best_unitindex;
				this->LastTargetPosition = best_pos;
			}
		}

		Vector dir = (best_pos - shootpos).Normal();
		EulerAngles ang = dir.Angles();

		pitchptr.WriteFast(ang.Pitch.AsRadians());
		yawptr.WriteFast(ang.Yaw.AsRadians());
	}
}

void Pringle::Aimbot::OnPreLocalPlayerInput(const Hooks::PreLocalPlayerInput & msg)
{
	if (!this->Shoot)
		return;

	if (this->ShotLast) // semi auto fix, spams shoot instead, todo, check if automatic gun
	{
		auto test = Blam::Input::GetActionState(Blam::Input::eGameActionFireRight);
		test->Msec = test->Ticks = 0;
		this->ShotLast = false;
	}
	else
	{
		auto test = Blam::Input::GetActionState(Blam::Input::eGameActionFireRight);
		test->Msec = test->Ticks = 1;
		this->ShotLast = true;
	}
}

