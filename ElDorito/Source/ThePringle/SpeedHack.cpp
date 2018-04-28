#include "Hooks.hpp"

#include "../Modules/ModuleBase.hpp"

using namespace Pringle;
using namespace Pringle::Hooks;
using namespace Modules;

// no one else needs our class, so we don't have to put it in the header.
// improves compile times and other stuff

class SpeedHack :
	public ModuleBase
{
public:
	SpeedHack() :
		ModuleBase("pringle")
	{
		Speed = this->AddVariableFloat("speed", "speed", "Player speed", eCommandFlagsNone);
		Speed->ValueFloat = Speed->DefaultValueFloat = 1.0f;

		Hook::SubscribeMember<ModifySpeedMultiplier>(this, &SpeedHack::SpeedMultiplier);
	}

protected:
	Command * Speed;

	void SpeedMultiplier(const ModifySpeedMultiplier& msg)
	{
		*msg.Speed *= Speed->ValueFloat;
	}
};

static SpeedHack SpeedHackInstance;