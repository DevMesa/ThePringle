#include "PringleCore.hpp"
#include "SpeedHack.hpp"
#include "Aimbot.hpp"

namespace Pringle
{
	void Core::Initialize()
	{
		SpeedHack::Instance();
		Aimbot::Initalize();
	}
}