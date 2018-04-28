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
		this->Factor = this->AddVariableFloat("speed.multiplier", "speed.multiplier", "Player speed multiplier", eCommandFlagsNone, 1.0f);
		this->Enabled = this->AddVariableInt("speed.enabled", "speed.enabled", "Enable the hack", eCommandFlagsNone, 0);

		Hook::SubscribeMember<ModifySpeedMultiplier>(this, &SpeedHack::OnModifySpeedMultiplier);
	}

protected:
	Command* Factor;
	Command* Enabled;

	void OnModifySpeedMultiplier(const ModifySpeedMultiplier& msg)
	{
		if(this->Enabled->ValueInt != 0)
			msg.Speed *= this->Factor->ValueFloat;
	}
};

static SpeedHack SpeedHackInstance;