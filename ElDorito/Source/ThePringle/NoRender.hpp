#pragma once

#ifndef __PRINGLE_NORENDER__
#define __PRINGLE_NORENDER__

#include "../Modules/ModuleBase.hpp"
#include "../Utils/Singleton.hpp"
#include "../Blam/BlamEvents.hpp"

#include "Hooks.hpp"

namespace Pringle
{
	class NoRenderMod :
		public Utils::Singleton<NoRenderMod>,
		public Modules::ModuleBase
	{
	public:
		NoRenderMod();

	protected:
		Modules::Command* Enabled;

		void OnRenderEffect(const Pringle::Hooks::RenderEffectEvent& event);
	};
}

#endif // !__PRINGLE_WELCOMEBOT__
