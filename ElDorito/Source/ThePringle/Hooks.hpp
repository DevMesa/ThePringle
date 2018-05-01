#pragma once
#ifndef PRINGLE_HOOKS
#define PRINGLE_HOOKS

#include <vector>
#include <functional>
#include <vector>
#include <algorithm>
#include <atomic>

#include "HookEvents.hpp"
#include "Properties.hpp"

namespace Pringle
{
	namespace HookPriority
	{
#define PRIORITY_OPTION(NAME, VALUE) \
	const float NAME = (VALUE); \
	struct NAME##Ex \
	{ \
		const float Pre   = (VALUE) - 0.5f; \
		const float Post  = (VALUE) + 0.5f; \
	}

		PRIORITY_OPTION(First, -2.0f);
		PRIORITY_OPTION(Before, -1.0f);
		PRIORITY_OPTION(Default, 0.0f);
		PRIORITY_OPTION(After, 1.0f);
		PRIORITY_OPTION(Last, 2.0f);
#undef PRIORITY_OPTION
	};

	class Hook
	{
		typedef unsigned long long HookId;
	private:
		template<typename T>
		struct SubscriptionInfo
		{
			std::function<void(const T&)> Function;
			float Priority;
			bool Enabled;
			HookId Id;

			SubscriptionInfo(std::function<void(const T&)> func, float priority) :
				Function(func), Priority(priority),
				Enabled(true)
			{
				static HookId NextId = 1;
				this->Id = NextId;
			}
		};

		template<typename T>
		struct HookInfo
		{
			std::vector<SubscriptionInfo<T>> Subscribed;
		};

		template<typename T>
		static HookInfo<T>& GetHookInfo()
		{
			static HookInfo<T> Info;

			return Info;
		}

		// this is called thru the hook handle
		template<typename T>
		static void Unsubscribe(HookId id)
		{
			HookInfo<T>& info = GetHookInfo<T>();
			for (std::vector<SubscriptionInfo<T>>::iterator it = info.Subscribed.begin(); it != info.Subscribed.end(); ++it)
			{
				if (it->Id != id)
					continue;

				info.Subscribed.erase(it);
				return;
			}
		}
	public:
		template<typename T, typename... Args>
		static void Call(Args&&... args)
		{
			CallPremade<T>(T(args...));
		}

		// for if you want a hook result
		template<typename T>
		static void CallPremade(const T& hook)
		{
			HookInfo<T>& info = GetHookInfo<T>();

			for (auto&& sub : info.Subscribed)
				if(sub.Enabled)
					sub.Function(hook);
		}

		template<typename T>
		class Handle;

		template<typename T>
		static Handle<T> Subscribe(std::function<void(const T&)>&& func, float priority = HookPriority::Default)
		{
			HookInfo<T>& info = GetHookInfo<T>();
			SubscriptionInfo<T>& subinf = info.Subscribed.emplace_back(func, priority);

			std::sort(info.Subscribed.begin(), info.Subscribed.end(), [](const SubscriptionInfo<T>& left, const SubscriptionInfo<T>& right) -> bool
			{
				return left.Priority < right.Priority;
			});

			return Handle<T>(subinf.Id);
		}

		template<typename T, typename What>
		static Handle<T> SubscribeMember(What* self, void (What::*member_func)(const T&), float priority = HookPriority::Default)
		{
			return Subscribe<T>(std::bind(member_func, self, std::placeholders::_1));
		}

		template<typename T>
		class Handle
		{
			friend class Hook;
		protected:

			struct HandleContainer
			{
				HandleContainer(HookId id) :
					Id(id),
					ReferenceCounter(0),
					Saved(false)
				{ }
				HookId Id;
				std::atomic<int> ReferenceCounter;
				bool Saved;
			};

			HandleContainer* Container;

			Handle(HookId id) :
				Container(new HandleContainer(id)),
				Enabled(
					[this](bool& stored, const bool& value)
					{
						HookInfo<T>& info = GetHookInfo<T>();
						for (auto& sub : info.Subscribed)
						{
							if (sub.Id == this->Container->Id)
							{
								sub.Enabled = value;
								return;
							}
						}
					},
					[this](bool& stored) -> bool
					{
						HookInfo<T>& info = GetHookInfo<T>();
						for (const auto& sub : info.Subscribed)
							if (sub.Id == this->Container->Id)
								return sub.Enabled;
						return false;
					}
				)
			{
			}
		public:
			Property<bool> Enabled;

			Handle() :
				Container(nullptr),
				Enabled(
					[](bool&, const bool&) {},
					[](bool&) -> bool { return false; }
				)
			{
			}

			Handle(const Handle<T>&& other) // copied by passing/returning/T x()/T x = /indirect copies
			{
				if (other.Container != nullptr)
					other.Container->ReferenceCounter++;

				this->Container = other.Container;
			}

			Handle<T>& operator =(Handle<T>&& other) // copied by an explicit = where the type was previously unknown
			{
				if (other.Container != nullptr)
					other.Container->ReferenceCounter++;

				this->Container = other.Container;
				this->Container->Saved = true;
				return *this;
			}

			~Handle()
			{
				if (this->Container == nullptr)
					return; // we never held onto an id
				if (this->Container->ReferenceCounter-- > 0)
					return; // someone still holds a reference

				if (this->Container->Saved) // if we've ever stored this value, unsub it
					Hook::Unsubscribe<T>(this->Container->Id);

				delete this->Container;
			}
		};
	};
}

#endif