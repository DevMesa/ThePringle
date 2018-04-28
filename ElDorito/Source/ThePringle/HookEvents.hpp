#pragma once
#ifndef PRINGLE_HOOKEVENTS
#define PRINGLE_HOOKEVENTS
namespace Pringle
{
	namespace Hooks
	{
		struct PreTick { };
		struct PostTick { };

		typedef PostTick Tick; // don't care which way, default to after
	}
}
#endif