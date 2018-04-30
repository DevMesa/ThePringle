#pragma once

#ifndef __PRINGLE_SERVICETAGMOD_
#define __PRINGLE_SERVICETAGMOD_

#include "../Modules/ModuleBase.hpp"
#include "../Utils/Singleton.hpp"

#include "Hooks.hpp"

namespace Pringle
{
	class ServiceTagMod :
		public Utils::Singleton<ServiceTagMod>,
		public Modules::ModuleBase
	{
	public:
		ServiceTagMod();

	protected:
		Modules::Command* Enabled;
		Modules::Command* Delay;
		Modules::Command* Text;

		void OnPostTick(const Hooks::PostTick& msg);
	};
}

#endif // !__PRINGLE_SERVICETAGMOD_
