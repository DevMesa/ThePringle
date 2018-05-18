#pragma once

#ifndef __PRINGLE_CUSTOMHOOKS_
#define __PRINGLE_CUSTOMHOOKS_

#include <d3d9.h>

namespace Pringle
{
	namespace CustomHooks
	{
		namespace DirectX
		{
			void Initialize(LPDIRECT3DDEVICE9 device);
		}

		namespace Halo
		{
			void Initialize();
		}
	}
}

#endif // !__PRINGLE_CUSTOMHOOKS_
