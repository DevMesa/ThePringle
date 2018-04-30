#pragma once

#ifndef __PRINGLE_ESP_
#define __PRINGLE_ESP_

#include "Hooks.hpp"

#include "../Modules/ModuleBase.hpp"
#include "../Utils/Singleton.hpp"

#include "../Blam/BlamObjects.hpp"

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
		void Draw(const Hooks::DirectX::EndScene & msg, Blam::Objects::ObjectBase* unit, uint32_t color);
		void DrawPlayers(const Hooks::DirectX::EndScene & msg);
		void OnEndScene(const Hooks::DirectX::EndScene& msg);
		void OnPreTick(const Hooks::PreTick & msg);

	private:
		std::vector<Blam::Objects::ObjectBase*> ally_player_units;
		std::vector<Blam::Objects::ObjectBase*> enemy_player_units;
	};
}

#endif // !__PRINGLE_ESP_
