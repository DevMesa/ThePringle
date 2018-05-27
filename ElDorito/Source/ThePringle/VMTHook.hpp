#pragma once

#ifndef __PRINGLE_VMTHOOK_
#define __PRINGLE_VMTHOOK_

#include <stdint.h>
#include <vector>
#include <memory>
#include <Windows.h>

namespace
{
	template <typename T>
	static bool IsInRange(T ptr)
	{
		return ptr != 0x0			// null
			&& ptr > 0x10000		// this is where the PE header is
			&& ptr < 0xFFF00000;	// 32bit program maximum memory range
	}

	class VProtectGuard
	{
		void* Address;
		uint32_t Size;
		DWORD Flags;
	public:
		VProtectGuard(void* address, uint32_t size, DWORD flags = PAGE_READWRITE) : Address(address), Size(size)
		{
			VirtualProtect(Address, Size, flags, &Flags);
		}
		~VProtectGuard()
		{
			DWORD _;
			VirtualProtect(Address, Size, Flags, &_);
		}
	};
}

namespace Pringle
{
	class VMTHook
	{
	private:
		uintptr_t** Interface_ptr;
		uintptr_t* VTableOriginal_ptr;
		std::unique_ptr<uintptr_t[]> VTable_ptr;
		uint32_t Size;
	public:
		VMTHook(void** interface_ptr, uint32_t size = 0U) :
			Interface_ptr(reinterpret_cast<uintptr_t**>(interface_ptr)), 
			VTableOriginal_ptr(*reinterpret_cast<uintptr_t**>(interface_ptr)),
			Size(size)
		{
			VProtectGuard guard(Interface_ptr, sizeof(uintptr_t*));

			if (!Size) while (IsInRange(VTableOriginal_ptr[Size])) ++Size;

			VTable_ptr = std::make_unique<uintptr_t[]>(Size);
			memcpy(VTable_ptr.get(), VTableOriginal_ptr, sizeof(uintptr_t) * Size);

			*Interface_ptr = VTable_ptr.get();
		}
		template <typename T>
		VMTHook(T interface_ptr, uint32_t count = 0U) : VMTHook(reinterpret_cast<void**>(interface_ptr), count) { }
		~VMTHook()
		{
			VProtectGuard guard(Interface_ptr, sizeof(uintptr_t*));
			*Interface_ptr = VTableOriginal_ptr;
		}

		template <typename T>
		inline T Get(uint32_t index) const
		{
			return reinterpret_cast<T>(VTable_ptr[index]);
		}

		template <typename T>
		inline T GetOriginal(uint32_t index) const
		{
			return reinterpret_cast<T>(VTableOriginal_ptr[index]);
		}

		void Hook(uint32_t index, void* func)
		{
			VTable_ptr[index] = reinterpret_cast<uintptr_t>(func);
		}

		void UnHook(uint32_t index)
		{
			VTable_ptr[index] = VTableOriginal_ptr[index];
		}
	};

	class HookGuard
	{
		VMTHook& Target;
		void* Hook_ptr;
		uint32_t Index;
	public:
		HookGuard(VMTHook& target, uint32_t index) : Target(target), Index(index), Hook_ptr(target.Get<void*>(index))
		{
			Target.UnHook(Index);
		}
		~HookGuard()
		{
			Target.Hook(Index, Hook_ptr);
		}
	};
}

#endif // !__PRINGLE_VMTHOOK_
