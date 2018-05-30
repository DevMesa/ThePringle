#pragma once

#ifndef __PRINGLE_AIMBOT_
#define __PRINGLE_AIMBOT_

#include "Hooks.hpp"

#include "../Modules/ModuleBase.hpp"
#include "../Utils/Singleton.hpp"

namespace Pringle
{
	class Aimbot :
		public Utils::Singleton<Aimbot>,
		public Modules::ModuleBase
	{
	public:
		Aimbot();

		bool HasTarget = false;
		uint32_t LastTargetUnitIndex = 0;
	protected:
		Vector LastSelfPosition;
		Vector LastTargetPosition;
		bool LastSelfPositionFresh;
		bool Shoot;
		bool ShotLast;

		Modules::Command* Enabled;
		Modules::Command* Passive;
		Modules::Command* AutoShoot;
		Modules::Command* X;
		Modules::Command* Y;

		Modules::Command* AimPos;

		Modules::Command* DistanceHalfAt;
		Modules::Command* DistanceImportance;

		Modules::Command* CenterImportance;

		Modules::Command* AliveImportance;
		Modules::Command* TeamImportance;
		Modules::Command* VisibleImportance;
		Modules::Command* HoldTargetImportance;
		Modules::Command* FOVImportance;

		Modules::Command* FOV;

		void OnTick(const Hooks::PostTick& msg);
		void OnPreLocalPlayerInput(const Hooks::PreLocalPlayerInput& msg);
		void GetPlayers(const Hooks::AimbotEvents::GetTargets& msg);

		void ScoreDistance(const Hooks::AimbotEvents::ScoreTarget& msg);
		void ScoreCenter(const Hooks::AimbotEvents::ScoreTarget& msg);
		void ScoreAlive(const Hooks::AimbotEvents::ScoreTarget& msg);
		void ScoreTeam(const Hooks::AimbotEvents::ScoreTarget& msg);
		void ScoreVisible(const Hooks::AimbotEvents::ScoreTarget& msg);
		void ScoreHoldPreviousTarget(const Hooks::AimbotEvents::ScoreTarget& msg);
		void ScoreFOV(const Hooks::AimbotEvents::ScoreTarget& msg);
	};
}

#endif 
