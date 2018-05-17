#pragma once

#include <cstdint>
#include <cassert>

//#include "../../Blam/Math/RealVector3D.hpp"
#include "Vector.hpp"

namespace Halo
{
	struct TraceResult
	{
		//Pringle::Vector N00000013; //0x0000
		int HitFlagsOrSurfaceMaybe;
		float NormalizedDistance; // 0 = startpos, 1 = endpos, 0.5 = midway
		Pringle::Vector HitPos; //0x0014
		int16_t unknownsad1;
		int16_t unknownsad2;
		// https://ht.transparencytoolkit.org/rcs-dev%5Cshare/HOME/cod/CRT/crt/src/concrt/StructuredWorkStealingQueue.h
		// https://github.com/neciudan/CudaProgramming/blob/master/ps1/utils.h
		// https://msdn.microsoft.com/en-us/library/dd504801.aspx
		// maybe a callback queue
		// concrt.h
		/*Concurrency::details::StructuredWorkStealingQueue<Concurrency::details::_UnrealizedChore,Concurrency::details::_CriticalNonReentrantLock>*/ void* MutexdWorkerQueue;
		int32_t unknownbasds;
		int16_t hit2; //0x0020
		int16_t hit3; //0x0022
		Pringle::Vector SurfaceNormal;
		float SurfaceNormalW; // idk, it just sets 4 soo
		int16_t hit4; //0x0034
		int32_t N00000046; //0x0036
		char pad_003A[6]; //0x003A
		int32_t N00000019; //0x0040
		int32_t oven; //0x0044
		int32_t N0000003A; //0x0048
		
		bool BoolA;
		bool BoolB;
		bool BoolC;
		bool BoolD;
		bool Hit_maybe;
		bool BoolF;

		int32_t N00000086; //0x0052
		int32_t N00000087; //0x0056
		char pad_005A[10]; //0x005A
		int32_t N0000001F; //0x0064
		int32_t N00000020; //0x0068
		char pad_006C[6]; //0x006C
		int32_t N00000061; //0x0072
		int32_t N00000024; //0x0076
		char pad_007A[22]; //0x007A
		int8_t N00000096; //0x0090
		char pad_0091[43]; //0x0091

	}; //Size: 0x00BC

	inline bool Trace(TraceResult& result, const Pringle::Vector& start, const Pringle::Vector& end, uint32_t unitIndex1, uint32_t unitIndex2)
	{
		Pringle::Vector direction_magnitude = end - start;

		uint32_t flags1 = (*(uint32_t*)0x471A8F8) | 0x200;
		uint32_t flags2 = *(uint32_t*)0x471A8FC;

		static const uint32_t addr = 0x6D7160;
		auto fn = reinterpret_cast<bool(*)(uint32_t, uint32_t, const Pringle::Vector*, const Pringle::Vector*, uint32_t, uint32_t, TraceResult*)> (addr);
		bool fn_result = fn(flags1, flags2, &start, &direction_magnitude, unitIndex1, unitIndex2, &result);

		return fn_result;
	}

	inline bool SimpleHitTest(const Pringle::Vector& start, const Pringle::Vector& end, uint32_t unitIndex1, uint32_t unitIndex2)
	{
		TraceResult result;

		Pringle::Vector direction_magnitude = end - start;

		uint32_t flags1 = (*(uint32_t*)0x471A8F8) | 0x200;
		uint32_t flags2 = *(uint32_t*)0x471A8FC;

		auto precopy1 = start;
		auto precopy2 = end;

		static const uint32_t addr = 0x6D7160;
		auto fn = reinterpret_cast<bool(*)(uint32_t, uint32_t, const Pringle::Vector*, const Pringle::Vector*, uint32_t, uint32_t, TraceResult*)> (addr);
		bool fn_result = fn(flags1, flags2, &start, &direction_magnitude, unitIndex1, unitIndex2, &result);

		assert(start.Y == precopy1.Y);
		assert(end.Y == precopy2.Y);

		return !fn_result;
	}
}