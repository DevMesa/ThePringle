#include "PringleCore.hpp"
#include "SpeedHack.hpp"
#include "ESP.hpp"
#include "Aimbot.hpp"
#include "ServiceTagMod.hpp"

namespace Pringle
{
	void Core::Initialize()
	{
		SpeedHack::Instance();
		Aimbot::Initalize();
		ESP::Instance();
		ServiceTagMod::Instance();
	}
}