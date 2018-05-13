#pragma once
#ifndef PRINGLE_HOOKS
#define PRINGLE_HOOKS

#include <type_traits>
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
			// could use overloading to achieve this, but that clutters
			// intellisense and makes it look like you can do it, well,
			// until you try and compile it
			if constexpr (sizeof...(Args) == 1)
			{
				static constexpr bool same_type = std::is_same_v< T, std::tuple_element_t<0, std::tuple<Args...>> >;
				static_assert(!same_type, "Attempting to use Hook::Call<T>(...) with an instantiated event. Use Call<T>(...) where ... is your constructor parameter list or use Hook::CallPremade<T>(const T&)");
			}

			CallPremade<T>(T(args...));
		}

	private:
		// for `using BaseType = T;` or `typedef T BaseType`
		template<typename T>
		struct HasBaseTypeTrait
		{
		private:
			template<typename U> static std::true_type _(typename U::BaseType*);
			template<typename U> static std::false_type _(...);
		public:
			static constexpr bool Value = decltype(_<T>(0))::value;
		};

		template<typename T>
		static constexpr bool HasBaseType = HasBaseTypeTrait<T>::Value;

	public:
		// for if you want a hook result
		template<typename T>
		static void CallPremade(const T& hook)
		{
			HookInfo<T>& info = GetHookInfo<T>();

			for (auto&& sub : info.Subscribed)
				if(sub.Enabled)
					sub.Function(hook);

			if constexpr (HasBaseType<T>)
			{
				static_assert(std::is_base_of_v<T::BaseType, T>, "T is not derived from T::BaseType");
				static_assert(!std::is_same_v<T, T::BaseType>, "T::BaseType can't be the same as T");

				// if is to prevent error spam if one of the above assertions fail
				if constexpr(std::is_base_of_v<T::BaseType, T> && !std::is_same_v<T, T::BaseType>)
					CallPremade<T::BaseType>(hook);
			}
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
			return Subscribe<T>(std::bind(member_func, self, std::placeholders::_1), priority);
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