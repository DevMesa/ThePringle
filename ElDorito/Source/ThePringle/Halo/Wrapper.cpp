#include "Wrapper.hpp"

namespace Pringle
{
	Entity::Entity(Blam::DatumHandle handle) :
		Uid(0),
		BlamHandle(handle)
	{
	}

	Blam::Objects::ObjectBase* Entity::BlamObject()
	{
		return nullptr;
	}

	void WrapperUpdate()
	{
		//Blam::Objects::GetObjects();
		auto& players = Blam::Players::GetPlayers();
		for (auto it = players.begin(); it != players.end(); it++)
		{

		}
	}
}