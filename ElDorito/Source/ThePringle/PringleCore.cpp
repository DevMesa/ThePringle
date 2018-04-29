#include "PringleCore.hpp"
#include "SpeedHack.hpp"

namespace Pringle
{
	void Core::Initialize()
	{
		SpeedHack::Instance();
	}
}