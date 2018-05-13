#include "Projectiles.hpp"

#include "../Modules/ModulePlayer.hpp"
#include "../Patches/PlayerRepresentation.hpp"
#include <sstream>
#include <algorithm> 

#include "../Blam/Tags/Game/Globals.hpp"
#include "../Blam/Tags/Items/DefinitionWeapon.hpp"
#include "../Patches/Forge.hpp"
#include "../Patches/Weapon.hpp"
#include "../Modules/ModuleForge.hpp"
#include "../Blam/BlamObjects.hpp"
#include "../Blam/BlamPlayers.hpp"
#include "../Blam/Tags/Tags.hpp"
#include "../Blam/Tags/Objects/Projectile.hpp"

using namespace Pringle::Hooks;
using namespace Modules;
using namespace Blam::Tags;
using namespace Patches::Weapon;

namespace Pringle
{
	TimeToImpactCalculator TTI;
	Projectiles::Projectiles()
		: ModuleBase("Pringle")
	{
		this->Enabled = this->AddVariableInt("Projectiles.Enabled", "projectiles.enabled", "Enable projectile support", eCommandFlagsNone /*TODO: make archive once working*/, 0);

		// make the hook first so we can divert the OnTarget
		Hook::SubscribeMember<AimbotEvents::ScoreTarget>(this, &Projectiles::FirstOnScoreTarget, HookPriority::First);

		TTI = this->CreateSimpleTimeToImpactCalculator(50.0f);
	}

	struct WeaponInfoEx : public WeaponInfo
	{
		float Scale;
	};

	static WeaponInfoEx GetEquippedWeaponEx(const Blam::DatumHandle& player_id)
	{
		WeaponInfoEx weapon;

		auto datum = Blam::Players::GetPlayers().Get(player_id);
		if (!datum)
			return weapon;

		auto object = Pointer(Blam::Objects::GetObjects()[datum->SlaveUnit].Data);
		if (!object)
			return weapon;
		
		auto equipped_weapon_index = object(0x2CA).Read<uint8_t>();
		if (equipped_weapon_index == -1)
			return weapon;

		auto equipped_weapon_object_index = object(0x2D0 + 4 * equipped_weapon_index).Read<uint32_t>();
		auto equipped_weapon_object_ptr = Pointer(Blam::Objects::GetObjects()[equipped_weapon_object_index].Data);
		if (!equipped_weapon_object_ptr)
			return weapon;

		auto obj = Blam::Objects::Get(datum->SlaveUnit.Handle);
		weapon.Scale = obj->Scale;

		weapon.Index = Pointer(equipped_weapon_object_ptr).Read<uint32_t>();
		weapon.Name = GetName(weapon);
		weapon.TagName = GetTagName(weapon);
		weapon.Offset = GetOffsets(weapon);
		return weapon;
	}

	void Projectiles::FirstOnScoreTarget(const Hooks::AimbotEvents::ScoreTarget& e)
	{
		if (!this->Enabled->ValueInt)
			return;

		auto weap = /*Patches::Weapon::*/GetEquippedWeaponEx(Blam::Players::GetLocalPlayer(0));
		auto* defweap = TagInstance(weap.Index).GetDefinition<Blam::Tags::Items::Weapon>();

		// what's defweap->Barrels[0].CrateProjectile

		if (!defweap || !defweap->Barrels)
			return;

		auto* defproj = defweap->Barrels[0].InitialProjectile.GetDefinition<Blam::Tags::Objects::Projectile>();

		if (!defproj)
			return;

		TTI = this->CreateSimpleTimeToImpactCalculator(defproj->FinalVelocity);
		
		// update the position of the target so we aim at that instead
		const Vector& pos = e.What.Position;
		const Vector& vel = e.What.Information.Velocity;
		const Vector acel = Vector::Down() * GameGlobals::Physics::DefaultGravity;

		float last_err = 1000.0f; // 1k seconds, should be high enough...
		float time = this->TimeToImpact(e.Self.Position, e.What.Position, TTI);

		// iteratively home in on the correct time-to-impact, up to a max of 16 iterations
		for (int i = 0; i < 16; i++)
		{
			const float was_time = time;

			Vector test_pos = this->EstimatePosition(pos, vel, acel, time);
			time = this->TimeToImpact(e.Self.Position, test_pos, TTI);

			const float err = std::abs(time - was_time);

			// if the error grows (not the time, only the error)
			// then it means we can't hit them (going quicker than the projectile away from us, for example)
			if (err > last_err)
			{
				e.Importance *= 0; // make it so we can't target them
				return;
			}
			else if (err <= 0.001) // error is less than 1 ms, it's good enough
				break;
			last_err = err;
		}
		
		e.What.Position = this->EstimatePosition(pos, vel, acel, time);

		// don't try to aim past the max range
		if ((e.What.Position - e.Self.Position).Length() > defproj->MaximumRange)
			e.Importance *= 0;
	}

	TimeToImpactCalculator Projectiles::CreateSimpleTimeToImpactCalculator(float projectile_speed)
	{
		return [=](const Vector& distance) -> float
		{
			return distance.Length() / projectile_speed;
		};
	}

	float Projectiles::TimeToImpact(const Vector& source, const Vector& position, TimeToImpactCalculator& tti)
	{
		Vector distance = (source - position);
		return tti(distance);
	}

	Vector Projectiles::EstimatePosition(const Vector& position, const Vector& velocity, const Vector& acceleration, float time)
	{
		return 
			position +                           // 
			velocity * time +                    // first derivative of position
			acceleration * time * time * 0.5f;   // second derivative of position (first derivative of first derivative of position)
	}

}