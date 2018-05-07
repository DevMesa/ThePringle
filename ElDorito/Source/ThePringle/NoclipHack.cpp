#include "NoclipHack.hpp"
#include "../Modules/ModulePlayer.hpp"
#include "../Patches/PlayerRepresentation.hpp"
#include <sstream>
#include <algorithm> 

#include "../Patches/Forge.hpp"
#include "../Modules/ModuleForge.hpp"
#include "../Blam/BlamObjects.hpp"
#include "../Blam/BlamPlayers.hpp"

using namespace Pringle;
using namespace Pringle::Hooks;
using namespace Modules;

NoclipHack::NoclipHack() : ModuleBase("pringle")
{
	this->Enabled = this->AddVariableInt("noclip.enabled", "noclip.enabled", "Enable the hack", eCommandFlagsArchived, 0);

	Hook::SubscribeMember<PreTick>(this, &NoclipHack::OnPreTick);
}

bool LastTickEnabled = false;

void NoclipHack::OnPreTick(const PreTick & msg)
{
	const auto& moduleForge = Modules::ModuleForge::Instance();
	if (moduleForge.VarMonitorNoclip->ValueInt != 0)
		return;

	const auto object_set_havok_flags = (int(*)(uint32_t objectIndex, uint32_t havokFlags))(0x005C7380);

	auto player = Blam::Players::GetPlayers().Get(Blam::Players::GetLocalPlayer(0));
	if (!player)
		return;

	auto unitObject = Blam::Objects::Get(player->SlaveUnit.Handle);
	if (!unitObject)
		return;

	bool Enabled = this->Enabled->ValueInt != 0;

	if (Enabled != LastTickEnabled) 
	{
		object_set_havok_flags(player->SlaveUnit, 0); // enable collisions, only done once when mod disabled
		LastTickEnabled = Enabled;
	}
	
	if (Enabled) 
	{
		auto havokComponents = *(Blam::DataArray<Blam::DatumBase>**)0x02446080;
		auto havokComponent = (uint8_t*)havokComponents->Get(unitObject->HavokComponent);
		if (!havokComponent)
			return;
		auto rigidBodyCount = *(uint32_t*)(havokComponent + 0x18);
		if (!rigidBodyCount)
			return;
		auto rigidBody = *(uint8_t**)(*(uint8_t**)(havokComponent + 0x14) + 0x40);
		auto &collisionFilter = *(uint8_t*)(rigidBody + 0x2c);

		collisionFilter = 0x13;
	}
}
