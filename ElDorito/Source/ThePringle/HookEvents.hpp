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
	}
}

#endif