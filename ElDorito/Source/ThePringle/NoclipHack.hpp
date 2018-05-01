#pragma once

#ifndef __PRINGLE_NOCLIPHACK__
#define __PRINGLE_NOCLIPHACK__

#include "../Modules/ModuleBase.hpp"
#include "../Utils/Singleton.hpp"

#include "Hooks.hpp"

namespace Pringle
{
	class NoclipHack :
		public Utils::Singleton<NoclipHack>,
		public Modules::ModuleBase
	{
	public:
		NoclipHack();

	protected:
		Modules::Command* Enabled;

		void OnPreTick(const Hooks::PreTick& msg);
	};
}

#endif // !__PRINGLE_NOCLIPHACK__
