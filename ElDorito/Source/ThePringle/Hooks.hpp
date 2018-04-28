#pragma once
#ifndef PRINGLE_HOOKS
#define PRINGLE_HOOKS

#include <vector>
#include <functional>
#include <vector>
#include <algorithm>

#include "HookEvents.hpp"

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
		template<typename T>
		struct SubscriptionInfo
		{
			float Priority;
			std::function<void(const T&)> Function;

			SubscriptionInfo(std::function<void(const T&)> func, float priority) :
				Function(func), Priority(priority)
			{
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

	public:
		template<typename T>
		static void Call(const T&& hook)
		{
			HookInfo<T>& info = GetHookInfo<T>();

			for (auto&& func : info.Subscribed)
				func.Function(hook);
		}

		template<typename T>
		static void Subscribe(std::function<void(const T&)>&& func, float priority = HookPriority::Default)
		{
			HookInfo<T>& info = GetHookInfo<T>();
			info.Subscribed.emplace_back(func, priority);

			std::sort(info.Subscribed.begin(), info.Subscribed.end(), [](const SubscriptionInfo<T>& left, const SubscriptionInfo<T>& right) -> bool
			{
				return left.Priority < right.Priority;
			});
		}
	};
}

#endif