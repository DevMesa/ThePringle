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

	protected:
		/*Vector CachedLocalPosition;
		int CachedTeam;
		Vector CachedAimDirection;*/

		Modules::Command* Enabled;
		Modules::Command* X;
		Modules::Command* Y;

		Modules::Command* AimPos;

		Modules::Command* DistanceHalfAt;
		Modules::Command* DistanceImportance;

		Modules::Command* CenterImportance;

		Modules::Command* AliveImportance;
		Modules::Command* TeamImportance;
		Modules::Command* VisibleImportance;

		void OnTick(const Hooks::PostTick& msg);
		void GetPlayers(const Hooks::AimbotEvents::GetTargets& msg);

		void ScoreDistance(const Hooks::AimbotEvents::ScoreTarget& msg);
		void ScoreCenter(const Hooks::AimbotEvents::ScoreTarget& msg);
		void ScoreAlive(const Hooks::AimbotEvents::ScoreTarget& msg);
		void ScoreTeam(const Hooks::AimbotEvents::ScoreTarget& msg);
		void ScoreVisible(const Hooks::AimbotEvents::ScoreTarget& msg);
	};
}

#endif 
