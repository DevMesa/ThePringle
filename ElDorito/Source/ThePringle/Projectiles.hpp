#pragma once

#ifndef PRINGLE_PROJECTILES
#define PRINGLE_PROJECTILES

#include "../Modules/ModuleBase.hpp"
#include "../Utils/Singleton.hpp"

#include "Hooks.hpp"

namespace Pringle
{
	using TimeToImpactCalculator = std::function<float(const Vector& distance)>;
	using PositionCalculator = std::function<float(float time)>;
	
	class Projectiles :
		public Utils::Singleton<Projectiles>,
		public Modules::ModuleBase
	{
	public:
		Projectiles();

	protected:
		Modules::Command* Enabled;
		void FirstOnScoreTarget(const Hooks::AimbotEvents::ScoreTarget& e);

		TimeToImpactCalculator CreateSimpleTimeToImpactCalculator(float projectile_speed);

		// calculates the time to impact for something not moving
		float TimeToImpact(const Vector& source, const Vector& position, TimeToImpactCalculator& tti);

		Vector EstimatePosition(const Vector& position, const Vector& velocity, const Vector& acceleration, float time);
	};
}

#endif // !PRINGLE_PROJECTILES
