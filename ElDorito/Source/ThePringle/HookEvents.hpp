#pragma once
#ifndef PRINGLE_HOOKEVENTS
#define PRINGLE_HOOKEVENTS

#include <d3d9.h>

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

		struct ModifyMarkerVisibility
		{
			bool& Visibility;
			ModifyMarkerVisibility(bool& visibility) : Visibility(visibility) { }
		};
	}
}

#endif