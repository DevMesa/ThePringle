#pragma once

#ifndef __PRINGLE_ESP_
#define __PRINGLE_ESP_

#include "Hooks.hpp"

#include "../Modules/ModuleBase.hpp"
#include "../Utils/Singleton.hpp"

namespace Pringle
{
	class ESP :
		public Utils::Singleton<ESP>,
		public Modules::ModuleBase
	{
	public:
		ESP();

		Modules::Command* Enabled;

	protected:
		void OnEndScene(const Hooks::DirectX::EndScene& msg);
	};
}

#endif // !__PRINGLE_ESP_
