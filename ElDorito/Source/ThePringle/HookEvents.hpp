#pragma once
#ifndef PRINGLE_HOOKEVENTS
#define PRINGLE_HOOKEVENTS

#include <vector>

#include <d3d9.h>

#include "Vector.hpp"
#include "QAngle.hpp"

#include "Draw.hpp"

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
				Pringle::Draw& Draw;

				EndScene(LPDIRECT3DDEVICE9 device) : Device(device), Draw(device) { }
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
		
		struct PreLocalPlayerInput { };
		struct PostLocalPlayerInput { };

		namespace AimbotEvents
		{
			struct AimPosition
			{
				enum Enum
				{
					Origin,
					Center,
					Head
				};
			};

			// returns all the valid targets we *could* attack
			// e.g: if friendly fire is off, don't return team members
			// but if it's on, return team members, even if team members are later ignored
			struct Target
			{
				struct Info
				{
					const Vector& Velocity;
					const Vector& ShootDirection;
					const QAngle& ViewAngles;

					const int Team;
					const uint32_t UnitIndex;
					const bool Alive : 1;
					const bool Player : 1;

					Info(const Vector& vel, const Vector& shootdir, const QAngle& angs, int team, int unitindex, bool alive, bool player) :
						Velocity(vel),
						ShootDirection(shootdir),
						ViewAngles(angs),
						Team(team),
						UnitIndex(unitindex),
						Alive(alive),
						Player(player)
					{
					}
				};

				// make a copy of this as it's mutable
				Vector Position;
				const Info& Information;

				Target(const Vector& pos, const Info& info) :
					Position(pos),
					Information(info)
				{}
			};

			struct ScoreTarget
			{
				const Target& Self;
				const Target& What;
				float& Importance;

				static float Calculate(float value, float importance)
				{
					// basically: interp(value, 1 - importance, 1)
					return (1.0f - importance) + value * importance;
				}

				ScoreTarget(const Target& self, const Target& what, float& out_importance) :
					Self(self),
					What(what),
					Importance(out_importance)
				{
					// the default value that will be scaled and shit
					// shouldn't really matter if it's 100 or 1,
					// but 0 to 100 is a nice range for humans reading it
					out_importance = 100.0f;
				}
			};

			struct GetTargets
			{
				// information about where you want to target them
				AimPosition::Enum                AimAt;
				std::function<void(Target& got)> GotTarget;

				GetTargets(const std::function<void(Target& got)>&& got_target, AimPosition::Enum aimat) : GotTarget(got_target), AimAt(aimat) {}
			};
		}

		struct ModifyMarkerVisibility
		{
			bool& Visibility;
			ModifyMarkerVisibility(bool& visibility) : Visibility(visibility) { }
		};
	}
}

#endif