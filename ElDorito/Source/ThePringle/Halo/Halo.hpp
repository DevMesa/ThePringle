#pragma once

#include <cstdint>

//#include "../../Blam/Math/RealVector3D.hpp"
#include "Vector.hpp"

namespace Halo
{
	struct TraceResult
	{
		Pringle::Vector N00000013; //0x0000
		char pad_000C[8]; //0x000C
		Pringle::Vector N00000016; //0x0014
		int16_t hit2; //0x0020
		int16_t hit3; //0x0022
		Pringle::Vector SurfaceNormal;
		char pad_0030[4]; //0x0030
		int16_t hit4; //0x0034
		int32_t N00000046; //0x0036
		char pad_003A[6]; //0x003A
		int32_t N00000019; //0x0040
		int32_t oven; //0x0044
		int32_t N0000003A; //0x0048
		char pad_004C[6]; //0x004C
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

		// these are custom
		Pringle::Vector HitPos;

	}; //Size: 0x00BC

	inline bool Trace(TraceResult& result, Pringle::Vector start, Pringle::Vector end, uint32_t unitIndex1, uint32_t unitIndex2)
	{
		end = result.N00000013 = end - start;

		uint32_t flags1 = (*(uint32_t*)0x471A8F8) | 0x200;
		uint32_t flags2 = *(uint32_t*)0x471A8FC;

		static const uint32_t addr = 0x6D7160;
		auto fn = reinterpret_cast<bool(*)(uint32_t, uint32_t, Pringle::Vector*, Pringle::Vector*, uint32_t, uint32_t, TraceResult*)> (addr);
		bool fn_result = fn(flags1, flags2, &start, &end, unitIndex1, unitIndex2, &result);

		// todo: this isn't right
		result.HitPos = start + result.N00000013;

		return fn_result;
	}

	inline bool SimpleHitTest(Pringle::Vector start, Pringle::Vector end, uint32_t unitIndex1, uint32_t unitIndex2)
	{
		TraceResult result;

		result.N00000013 = end - start;
		end = result.N00000013;

		uint32_t flags1 = (*(uint32_t*)0x471A8F8) | 0x200;
		uint32_t flags2 = *(uint32_t*)0x471A8FC;

		static const uint32_t addr = 0x6D7160;
		auto fn = reinterpret_cast<bool(*)(uint32_t, uint32_t, Pringle::Vector*, Pringle::Vector*, uint32_t, uint32_t, TraceResult*)> (addr);
		bool fn_result = fn(flags1, flags2, &start, &end, unitIndex1, unitIndex2, &result);

		return !fn_result;
	}

	/*inline bool SimpleHitTest(Pringle::Vector* start, Pringle::Vector* end, uint32_t unitIndex1, uint32_t unitIndex2)
	{
		TraceResult result;

		result.N00000013 = *end - *start;
		*end = result.N00000013;

		uint32_t flags1 = (*(uint32_t*)0x471A8F8) | 0x200;
		uint32_t flags2 = *(uint32_t*)0x471A8FC;

		static const uint32_t addr = 0x6D7160;
		auto fn = reinterpret_cast<bool(*)(uint32_t, uint32_t, Pringle::Vector*, Pringle::Vector*, uint32_t, uint32_t, TraceResult*) > (addr);
		bool fn_result = fn(flags1, flags2, start, end, unitIndex1, unitIndex2, &result);

		return !fn_result;
	}*/
}