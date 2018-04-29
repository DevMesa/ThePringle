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
		Command* Enabled;
		Command* X;
		Command* Y;

		void OnTick(const PostTick& msg);

		std::vector<GetTargets::Target> CachedTargets;
		void OnGetTargets(const GetTargets& msg);
	};
}

#endif 
