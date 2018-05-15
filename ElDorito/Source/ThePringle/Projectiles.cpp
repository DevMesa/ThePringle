#include "Projectiles.hpp"

#include <sstream>
#include <algorithm> 

#include "../Patches/PlayerRepresentation.hpp"
#include "../Patches/Forge.hpp"
#include "../Patches/Weapon.hpp"
#include "../Modules/ModuleForge.hpp"
#include "../Modules/ModulePlayer.hpp"
#include "../Blam/BlamTime.hpp"
#include "../Blam/BlamObjects.hpp"
#include "../Blam/BlamPlayers.hpp"
#include "../Blam/Tags/Tags.hpp"
#include "../Blam/Tags/Objects/Projectile.hpp"
#include "../Blam/Tags/Game/Globals.hpp"
#include "../Blam/Tags/Items/DefinitionWeapon.hpp"
#include "Halo/Halo.hpp"

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
		this->Enabled = this->AddVariableInt("Projectiles.Enabled", "projectiles.enabled", "Enable projectile support", eCommandFlagsArchived, 1);
		this->MaxTimeToImpact = this->AddVariableFloat("Projectiles.Max.TimeToImpact", "projectiles.max.timetoimpact", "Maximum time to impact in seconds", eCommandFlagsArchived, 1.0);
		this->MaxJerk = this->AddVariableFloat("Projectiles.Max.Jerk", "projectiles.max.jerk", "Maximum jerk", eCommandFlagsArchived, 0.5);

		// make the hook first so we can divert the OnTarget
		Hook::SubscribeMember<Tick>(this, &Projectiles::TrackDerivatives, HookPriority::First);
		Hook::SubscribeMember<AimbotEvents::ScoreTarget>(this, &Projectiles::FirstOnScoreTarget, HookPriority::First);

		Vector src(0, 0, 10);
		Vector dst(50, 0, -10);
		Vector dir = (dst - src).Normal();

		Vector hitnorm(0, 0, 1);
		Vector hitpos = src + dir * 5.0f;

		float hitpos_end_distance = (hitpos - dst).Length();

		Vector goodpos = hitpos + dir.ProjectOnPlane(hitnorm) * hitpos_end_distance;

		TTI = this->CreateSimpleTimeToImpactCalculator(50.0f);
	}

	std::unordered_map<uint32_t, ProjectileDerivativeCalculator*> DerivativeCalculators;
	ProjectileDerivativeCalculator* Projectiles::GetPositionDerivatives(uint32_t id)
	{
		auto it = DerivativeCalculators.find(id);
		if (it == DerivativeCalculators.end())
			return nullptr;
		return it->second;
	}

	void Projectiles::TrackDerivatives(const Tick& e)
	{
		if (!this->Enabled->ValueInt)
			return;

		float time = Blam::Time::GetSecondsPerTick();
		uint32_t ticks = Blam::Time::GetGameTicks();
		
		if (ticks == this->LastTicks)
			return;
		this->LastTicks = ticks;

		auto new_player_pos = [&](uint32_t index, const Vector& position)
		{
			auto it = DerivativeCalculators.find(index);

			if (it == DerivativeCalculators.end())
			{
				ProjectileDerivativeCalculator* ptr = DerivativeCalculators[index] = new ProjectileDerivativeCalculator();
				ptr->Derivatives[0] = position;
			}
			else
				it->second->Update(position, time);
		};

		// standard player iterate

		auto& players = Blam::Players::GetPlayers();
		auto& localplayerhandle = Blam::Players::GetLocalPlayer(0);

		for (auto it = players.begin(); it != players.end(); ++it)
		{
			const auto& unitObjectIndex = it->SlaveUnit.Handle;

			if (unitObjectIndex == -1)
				continue;

			auto unit = Blam::Objects::Get(unitObjectIndex);

			if (!unit)
				continue;

			new_player_pos(unitObjectIndex, unit->Position);
		}
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

		if (!defweap || !defweap->Barrels || defweap->Barrels.Count == 0)
			return;

		auto* defproj = defweap->Barrels[0].InitialProjectile.GetDefinition<Blam::Tags::Objects::Projectile>();

		if (!defproj)
			return;

		TTI = this->CreateSimpleTimeToImpactCalculator(defproj->FinalVelocity);
		
		// update the position of the target so we aim at that instead
		Vector pos = e.What.Position;
		Vector vel = e.What.Information.Velocity;
		Vector acel = Vector::Zero();

		auto dc = GetPositionDerivatives(e.What.Information.UnitIndex);
		if (dc)
		{
			vel = dc->Velocity();
			acel = dc->Acceleration();
			//auto jerk = dc->Jerk();
		}

		float last_err = 1000.0f; // 1k seconds, should be high enough...
		float time = this->TimeToImpact(e.Self.Position, e.What.Position, TTI);

		// iteratively home in on the correct time-to-impact, up to a max of 16 iterations
		for (int i = 0; i < 16; i++)
		{
			const float was_time = time;

			pos = this->EstimatePosition(e.What.Position, vel, acel, time, false, e);
			time = this->TimeToImpact(e.Self.Position, pos, TTI);

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
		
		// don't try to aim past the max range
		if (time > this->MaxTimeToImpact->ValueFloat || (pos - e.Self.Position).Length() > defproj->MaximumRange)
		{
			e.Importance *= 0;
			return;
		}
		
		// do 1 more manual iteration, but with tracing on so we don't shoot thru walls
		// and then update their final pos with one more iteration
		{
			pos = this->EstimatePosition(e.What.Position, vel, acel, time, true, e);
			time = this->TimeToImpact(e.Self.Position, pos, TTI);

			e.What.Position = this->EstimatePosition(e.What.Position, vel, acel, time, true, e);
		}
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

	Vector Projectiles::EstimatePosition(const Vector& position, const Vector& velocity, const Vector& acceleration, float time, bool trace, const Hooks::AimbotEvents::ScoreTarget& e)
	{
		Vector estimated = 
			position +                           // 
			velocity * time +                    // first derivative of position
			acceleration * time * time * 0.5f;   // second derivative of position (first derivative of first derivative of position)

		// do a trace from position, to estimated position, to see if it collides with anything,
		// if it does, correct it to the surface normal of what the trace hit

		if (trace)
		{
			const float step_height = 0.1f; // TODO: put a real value here
			const Vector offset = e.What.Information.OriginOffset + Vector::Up() * step_height; // add a bit 

			Halo::TraceResult trace;
			if (Halo::Trace(trace, position - offset, estimated - offset, e.What.Information.UnitIndex, -1))
			{
				Vector dir = (estimated - position).Normal();
				float hitpos_est_distance = (trace.HitPos - estimated).Length();

				estimated = trace.HitPos + dir.ProjectOnPlane(trace.SurfaceNormal) * hitpos_est_distance;
				estimated += offset; // restore the offset
			}
		}

		return estimated;
	}

}