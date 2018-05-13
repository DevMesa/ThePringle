#pragma once

#ifndef PRINGLE_PROJECTILES
#define PRINGLE_PROJECTILES

#include "../Modules/ModuleBase.hpp"
#include "../Utils/Singleton.hpp"

#include "Hooks.hpp"

namespace Pringle
{
	template<typename T, size_t Size, size_t AverageSamples>
	struct DerivativeCalculator
	{
		// pos, vel, acel, jerk, snap
		T Derivatives[Size];
		T Last[Size][AverageSamples + 1];

		T Averaged[Size];

		// calculate the new derivatives
		void Update(const T& newpos, float deltatime)
		{
			// calculate the averages
			for (int i = 0; i < Size; i++)
				for (int a = AverageSamples; a --> 0;)
					Last[i][a + 1] = Last[i][a];

			for (int i = 0; i < Size; i++)
				Last[i][0] = Derivatives[i];

			Derivatives[0] = newpos;
			for (int i = 1; i < Size; i++)
			{
				Derivatives[i] = (Derivatives[i - 1] - Last[i - 1][0]) / deltatime;

				Vector average = Derivatives[i];
				for (int a = 0; a < AverageSamples; a++)
					average += Last[i][a];
				average /= (float)(AverageSamples + 1);
				Averaged[i] = average;
			}
		}

		const T& Velocity()
		{
			static_assert(Size >= 2, "Velocity is the 1st derivative, have less");
			return Averaged[1];
		}

		const T& Acceleration()
		{
			static_assert(Size >= 3, "Acceleration is the 2nd derivative, have less");
			return Averaged[2];
		}

		const T& Jerk()
		{
			static_assert(Size >= 4, "Velocity is the 3rd derivative, have less");
			return Averaged[3];
		}

		const T& Snap()
		{
			static_assert(Size >= 5, "Snap is the 4th derivative, have less");
			return Averaged[4];
		}
	};

	using ProjectileDerivativeCalculator = DerivativeCalculator<Vector, 4, 4>;

	using TimeToImpactCalculator = std::function<float(const Vector& distance)>;
	using PositionCalculator = std::function<float(float time)>;
	
	class Projectiles :
		public Utils::Singleton<Projectiles>,
		public Modules::ModuleBase
	{
	public:
		Projectiles();

	protected:
		uint32_t LastTicks;
		Modules::Command* Enabled;
		Modules::Command* MaxTimeToImpact;
		Modules::Command* MaxAcceleration;
		ProjectileDerivativeCalculator* GetPositionDerivatives(uint32_t id);
		void TrackDerivatives(const Pringle::Hooks::Tick& e);
		void FirstOnScoreTarget(const Hooks::AimbotEvents::ScoreTarget& e);

		TimeToImpactCalculator CreateSimpleTimeToImpactCalculator(float projectile_speed);

		// calculates the time to impact for something not moving
		float TimeToImpact(const Vector& source, const Vector& position, TimeToImpactCalculator& tti);

		Vector EstimatePosition(const Vector& position, const Vector& velocity, const Vector& acceleration, float time);
	};
}

#endif // !PRINGLE_PROJECTILES
