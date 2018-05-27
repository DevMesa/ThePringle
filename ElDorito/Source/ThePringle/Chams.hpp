#pragma once

#ifndef __PRINGLE_CHAMS_
#define __PRINGLE_CHAMS_

#include "Hooks.hpp"

#include "../Modules/ModuleBase.hpp"
#include "../Utils/Singleton.hpp"
#include "../CommandMap.hpp"

namespace Pringle
{
	class Chams : public Utils::Singleton<Chams>, public Modules::ModuleBase
	{
	public:
		Modules::Command* Enabled;

		Chams();

	private:
		void OnDirectXInitialize(const Hooks::DirectX::Initialize& msg);
		void OnBeginScene(const Hooks::DirectX::BeginScene& msg);
		void OnDrawObjectPrimitive(const Hooks::DirectX::DrawObjectPrimitive& msg);
	};
}

#endif // !__PRINGLE_CHAMS_
