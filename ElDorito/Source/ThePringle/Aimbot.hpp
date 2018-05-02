#pragma once

#ifndef __PRINGLE_AIMBOT_
#define __PRINGLE_AIMBOT_

#include "Hooks.hpp"

#include "../Modules/ModuleBase.hpp"
#include "../Utils/Singleton.hpp"

using namespace Pringle;
using namespace Pringle::Hooks;
using namespace Modules;

namespace Pringle
{
	class Aimbot :
		public Utils::Singleton<Aimbot>,
		public ModuleBase
	{
	public:
		static void Initalize();
		Aimbot();

	protected:
		Vector CachedLocalPosition;
		int CachedTeam;
		QAngle CachedViewAngles;

		Command* Enabled;
		Command* X;
		Command* Y;

		Command* DistanceHalfAt;
		Command* DistanceImportance;

		Command* CenterImportance;

		Command* AliveImportance;
		Command* TeamImportance;

		void OnTick(const PostTick& msg);
		void GetPlayers(const AimbotEvents::GetTargets& msg);

		void ScoreDistance(const AimbotEvents::ScoreTarget& msg);
		void ScoreCenter(const AimbotEvents::ScoreTarget& msg);
		void ScoreAlive(const AimbotEvents::ScoreTarget& msg);
		void ScoreTeam(const AimbotEvents::ScoreTarget& msg);
	};
}

#endif 
