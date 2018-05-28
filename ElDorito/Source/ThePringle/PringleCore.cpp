#include "PringleCore.hpp"
#include "SpeedHack.hpp"
#include "ESP.hpp"
#include "Aimbot.hpp"
#include "Projectiles.hpp"
#include "ServiceTagMod.hpp"
#include "NoclipHack.hpp"
#include "FontManager.hpp"
#include "NoRender.hpp"
#include "Chams.hpp"

#include "../Console.hpp"
#include "../Patch.hpp"

namespace Pringle
{
	void Core::Initialize()
	{
		SpeedHack::Instance();
		Aimbot::Instance();
		Projectiles::Instance();
		FontManager::Instance();
		ESP::Instance();
		ServiceTagMod::Instance();
		NoclipHack::Instance();
		NoRenderMod::Instance();
		Chams::Instance();
	}
}