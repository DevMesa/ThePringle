#include "PringleCore.hpp"
#include "SpeedHack.hpp"
#include "ESP.hpp"
#include "Aimbot.hpp"
#include "Projectiles.hpp"
#include "ServiceTagMod.hpp"
#include "NoclipHack.hpp"
#include "FontManager.hpp"
#include "NoRender.hpp"

#include "../Console.hpp"
#include "../Patch.hpp"


#include "Halo/Wrapper.hpp"

void TickHook()
{
	Pringle::Hook::Call<Pringle::Hooks::PreTick>();

	auto sub_B36510 = reinterpret_cast<void(*)()>(0xB36510);
	sub_B36510();

	Pringle::Hook::Call<Pringle::Hooks::PostTick>();
}

//B365F4
void TickHook2()
{
	Pringle::Hook::Call<Pringle::Hooks::PreTick>();

	auto sub_B69AD0 = reinterpret_cast<void(*)()>(0xB69AD0);
	sub_B69AD0();

	Pringle::WrapperUpdate();

	Pringle::Hook::Call<Pringle::Hooks::PostTick>();
}

void SetupTickHook()
{
	Hook(0xB365F4 - 0x400000, TickHook2, HookFlags::IsCall).Apply();
}

namespace Pringle
{
	void Core::Initialize()
	{
		SetupTickHook();

		SpeedHack::Instance();
		Aimbot::Instance();
		Projectiles::Instance();
		FontManager::Instance();
		ESP::Instance();
		ServiceTagMod::Instance();
		NoclipHack::Instance();
		NoRenderMod::Instance();
	}
}