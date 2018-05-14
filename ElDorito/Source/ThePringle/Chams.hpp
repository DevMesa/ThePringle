#pragma once

#ifndef __PRINGLE_CHAMS_
#define __PRINGLE_CHAMS_

#include "../Modules/ModuleBase.hpp"
#include "../Utils/Singleton.hpp"
#include "../CommandMap.hpp"

namespace Pringle
{
	class Chams : public Utils::Singleton<Chams>, public Modules::ModuleBase
	{
	public:
		Chams();

	//private:
		Modules::Command* Upper;
		Modules::Command* Lower;
		Modules::Command* Only;
	};
}

#endif // !__PRINGLE_CHAMS_
