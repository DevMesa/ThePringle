#pragma once
#ifndef PRINGLE_HOOKEVENTS
#define PRINGLE_HOOKEVENTS
namespace Pringle
{
	namespace Hooks
	{
		struct Tick
		{
			size_t Count;
			Tick(size_t count) : Count(count) {}
		};
	}
}
#endif