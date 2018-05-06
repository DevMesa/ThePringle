#pragma once

#ifndef __PRINGLE_AIMBOT_
#define __PRINGLE_AIMBOT_

#include "Hooks.hpp"

#include "../Modules/ModuleBase.hpp"
#include "../Utils/Singleton.hpp"

//using namespace Pringle;
//using namespace Pringle::Hooks;
//using namespace Modules;

namespace Pringle
{
	class Aimbot :
		public Utils::Singleton<Aimbot>,
		public Modules::ModuleBase
	{
	public:
		static void Initalize();
		Aimbot();

	protected:
		Modules::Command* Enabled;
		Modules::Command* X;
		Modules::Command* Y;

		bool HitTest(Blam::Math::RealVector3D start, Blam::Math::RealVector3D end);

		void OnTick(const Pringle::Hooks::PostTick& msg);

		std::vector<Pringle::Hooks::GetTargets::Target> CachedTargets;
		void OnGetTargets(const Pringle::Hooks::GetTargets& msg);
	};
}

#endif 
