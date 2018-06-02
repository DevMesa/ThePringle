#pragma once
#ifndef PRINGLE_HALO_WRAPPER
#define PRINGLE_HALO_WRAPPER

#include "../../Blam/BlamObjects.hpp"
#include "../../Blam/BlamPlayers.hpp"

#include <unordered_map>
#include <list>
#include <functional>

namespace Pringle
{
	class Entity
	{
	protected:
		Entity() = delete;
		Entity(Blam::DatumHandle handle);

		template<typename T>
		static std::unordered_map<int32_t, T> GlobalExtensionHashtable();

		std::list<std::function<void()>> ExtensionDeleters;
	public:
		const int Uid;
		const Blam::DatumHandle BlamHandle;
		Blam::Objects::ObjectBase* BlamObject();

		template<typename T>
		T& Extension();

		template<typename T>
		bool TryGetExtension(T*& result);

		template<typename T>
		bool TryGetExtension(const T*& result) const;
	};


	template<typename T>
	inline std::unordered_map<int32_t, T> Entity::GlobalExtensionHashtable()
	{
		static std::unordered_map<int32_t, T> table;
		return table;
	}

	template<typename T>
	inline T& Entity::Extension()
	{
		std::unordered_map<int32_t, T>& table =
			Entity::GlobalExtensionHashtable<T>();
		auto ext = table.find(this->BlamHandle.Handle);

		if (ext != table.end())
			return ext->second;
		else
		{
			T& ret = table.emplace(this->BlamHandle.Handle, this)->first->second;

			this->ExtensionDeleters += [&table, this]()
			{
				table.erase(this->BlamHandle.Handle);
			};

			return ret;
		}
	}

	template<typename T>
	inline bool Entity::TryGetExtension(T*& got)
	{
		auto ext = Entity::GlobalExtensionHashtable<T>()
			.find(this->BlamHandle.Handle);
		if (ext == table.end())
		{
			got = nullptr;
			return false;
		}
		got = &ext->second;
		return true;
	}
	
	template<typename T>
	inline bool Entity::TryGetExtension(const T*& got) const
	{
		auto ext = Entity::GlobalExtensionHashtable<T>()
			.find(this->BlamHandle.Handle);
		if (ext == table.end())
		{
			got = nullptr;
			return false;
		}
		got = &ext->second;
		return true;
	}

	void WrapperUpdate();
}

#endif