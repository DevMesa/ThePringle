#include "SpeedHack.hpp"

using namespace Pringle;
using namespace Pringle::Hooks;
using namespace Modules;

SpeedHack::SpeedHack() : ModuleBase("Pringle")
{
	this->Factor = this->AddVariableFloat("Speed.Multiplier", "speed.multiplier", "Player speed multiplier", eCommandFlagsArchived, 1.0f);
	this->Enabled = this->AddVariableInt("Speed.Enabled", "speed.enabled", "Enable the hack", eCommandFlagsArchived, 0);

	this->EnableAirAcceleration = this->AddVariableInt("Speed.AirAccelerate.Enabled", "speed.airaccelerate.enabled", "Enable the hack", eCommandFlagsArchived, 0);
	this->AirAcceleration = this->AddVariableFloat("Speed.AirAccelerate.Value", "speed.airaccelerate.value", "Allows you to fly in the air", eCommandFlagsArchived, 999.f);

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
	if (this->EnableAirAcceleration->ValueInt != 0)
	{
		if (this->Enabled->ValueInt != 0)
			msg.AirborneAcceleration = this->AirAcceleration->ValueFloat;
		else
			msg.AirborneAcceleration = msg.GroundAcceleration;
	}
}
