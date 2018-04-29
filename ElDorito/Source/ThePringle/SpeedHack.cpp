#include "SpeedHack.hpp"

using namespace Pringle;
using namespace Pringle::Hooks;
using namespace Modules;

// no one else needs our class, so we don't have to put it in the header.
// improves compile times and other stuff

SpeedHack::SpeedHack() : ModuleBase("pringle")
{
	this->Factor = this->AddVariableFloat("speed.multiplier", "speed.multiplier", "Player speed multiplier", eCommandFlagsArchived, 1.0f);
	this->Enabled = this->AddVariableInt("speed.enabled", "speed.enabled", "Enable the hack", eCommandFlagsArchived, 0);

	this->EnableAirAcceleration = this->AddVariableInt("speed.airaccelerate.enabled", "speed.airaccelerate.enabled", "Enable the hack", eCommandFlagsArchived, 0);
	this->AirAcceleration = this->AddVariableFloat("speed.airaccelerate.value", "speed.airaccelerate.value", "Allows you to fly in the air", eCommandFlagsArchived, 999.f);

	Hook::SubscribeMember<ModifySpeedMultiplier>(this, &SpeedHack::OnModifySpeedMultiplier);
	Hook::SubscribeMember<ModifyAcceleration>(this, &SpeedHack::OnModifyAcceleration);
}

void SpeedHack::OnModifySpeedMultiplier(const ModifySpeedMultiplier & msg)
{
	if (this->Enabled->ValueInt != 0)
		msg.Speed *= this->Factor->ValueFloat;
}

void SpeedHack::OnModifyAcceleration(const ModifyAcceleration & msg)
{
	if (this->Enabled->ValueInt != 0 && this->EnableAirAcceleration->ValueInt != 0)
		msg.AirborneAcceleration = this->AirAcceleration->ValueFloat;
}
