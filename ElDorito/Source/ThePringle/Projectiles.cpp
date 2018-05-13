#include "Projectiles.hpp"

#include "../Modules/ModulePlayer.hpp"
#include "../Patches/PlayerRepresentation.hpp"
#include <sstream>
#include <algorithm> 

#include "../Patches/Forge.hpp"
#include "../Modules/ModuleForge.hpp"
#include "../Blam/BlamObjects.hpp"
#include "../Blam/BlamPlayers.hpp"

using namespace Pringle::Hooks;
using namespace Modules;

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

	void Projectiles::FirstOnScoreTarget(const Hooks::AimbotEvents::ScoreTarget& e)
	{
		if (!this->Enabled->ValueInt)
			return;

		// update the position of the target so we aim at that instead
		const Vector& pos = e.What.Position;
		const Vector& vel = e.What.Information.Velocity;
		const Vector acel = Vector::Zero(); // TODO: account for this (gravity, basically, but don't go thru floors)

		float last_err = 1000.0f; // 1k seconds, should be high enough...
		float time = this->TimeToImpact(e.Self.Position, e.What.Position, TTI);

		// iteratively home in on the correct time-to-impact, up to a max of 32 iterations
		for (int i = 0; i < 32; i++)
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