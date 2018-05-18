#pragma once
#ifndef PRINGLE_HALO_WRAPPER
#define PRINGLE_HALO_WRAPPER

#include "../../Blam/BlamObjects.hpp"
#include "../../Blam/BlamPlayers.hpp"

namespace Pringle
{
	class Entity
	{
	public:
		int Uid;
		Blam::DatumHandle BlamHandle;
		Blam::Objects::ObjectBase* BlamObject();
	};
}

#endif