#pragma once

#ifndef __PRINGLE_SPEEDHACK_
#define __PRINGLE_SPEEDHACK_

#include "../Modules/ModuleBase.hpp"
#include "../Utils/Singleton.hpp"

#include "Hooks.hpp"

namespace Pringle
{
	class SpeedHack :
		public Utils::Singleton<SpeedHack>,
		public Modules::ModuleBase
	{
	public:
		SpeedHack();

	protected:
		Modules::Command* Factor;
		Modules::Command* Enabled;

		Modules::Command* EnableAirAcceleration;
		Modules::Command* AirAcceleration;

		void OnModifySpeedMultiplier(const Hooks::ModifySpeedMultiplier& msg);
		void OnModifyAcceleration(const Hooks::ModifyAcceleration& msg);
	};
}

#endif // !__PRINGLE_SPEEDHACK_
