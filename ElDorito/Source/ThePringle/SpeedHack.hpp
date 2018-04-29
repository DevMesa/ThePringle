#pragma once

#ifndef __PRINGLE_SPEEDHACK_
#define __PRINGLE_SPEEDHACK_

#include "Hooks.hpp"

#include "../Modules/ModuleBase.hpp"
#include "../Utils/Singleton.hpp"

using namespace Pringle;
using namespace Pringle::Hooks;
using namespace Modules;

namespace Pringle
{
	class SpeedHack :
		public Utils::Singleton<SpeedHack>,
		public ModuleBase
	{
	public:
		SpeedHack();

	protected:
		Command* Factor;
		Command* Enabled;

		Command* EnableAirAcceleration;
		Command* AirAcceleration;

		void OnModifySpeedMultiplier(const ModifySpeedMultiplier& msg);
		void OnModifyAcceleration(const ModifyAcceleration& msg);
	};
}

#endif // !__PRINGLE_SPEEDHACK_
