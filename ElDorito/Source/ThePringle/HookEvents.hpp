#pragma once
#ifndef PRINGLE_HOOKEVENTS
#define PRINGLE_HOOKEVENTS

#include <d3d9.h>
#include "Blam/Math/RealVector3D.hpp"
#include <vector>

namespace Pringle
{
	namespace Hooks
	{
		struct PreTick { };
		struct PostTick { };

		typedef PostTick Tick; // don't care which way, default to after

		namespace DirectX
		{
			struct EndScene
			{
				LPDIRECT3DDEVICE9 Device;
				EndScene(LPDIRECT3DDEVICE9 device) : Device(device) {}
			};
			struct PreReset {};
			struct PostReset {};
		}

		struct ModifySpeedMultiplier
		{
			float& Speed;
			ModifySpeedMultiplier(float& speed) : Speed(speed) { }
		};

		struct ModifyAcceleration
		{
			float& GroundAcceleration;
			float& AirborneAcceleration;
			ModifyAcceleration(float& ground_acceleration, float& airborn_acceleration) : GroundAcceleration(ground_acceleration), AirborneAcceleration(airborn_acceleration) { }
		};

		struct ModifyGravityMultiplier
		{
			float& Gravity;
			ModifyGravityMultiplier(float& gravity) : Gravity(gravity) { }
		};

		struct GetTargets
		{
			struct AimPosition
			{
				enum Enum
				{
					CenterMass,
					Head
				};
			};

			struct Target
			{
				Blam::Math::RealVector3D Position;
				Blam::Math::RealVector3D Velocity; // projectiles will need this
				AimPosition::Enum AimPart;			
				float Priority;

				Target(
					const Blam::Math::RealVector3D& pos,
					const Blam::Math::RealVector3D& vel,
					AimPosition::Enum                aimpart   = AimPosition::CenterMass,
					float                            priority  = 0.0f
				) :
					Position(pos),
					Velocity(vel),
					AimPart(aimpart),
					Priority(priority)
				{};
			};

			// reference to a list to save allocations
			std::vector<Target>& Targets;

			GetTargets(decltype(Targets) targets) : Targets(targets) {}
		};
	}
}

#endif