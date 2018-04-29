
#include "EulerAngle.hpp"

namespace Pringle
{
	template<typename T>
	T const pi = std::acos(-T(1));

	Angle Angle::Degrees(float deg)
	{
		return Angle((float)(deg / 180.0f * pi<float>));
	}

	Angle Angle::Radians(float rad)
	{
		return Angle(rad);
	}

	Angle::Angle() :
		R(0)
	{
	}

	Angle::Angle(float radians) :
		R(radians)
	{
	}

	EulerAngle::EulerAngle() :
		EulerAngle(0_deg, 0_deg, 0_deg)
	{
	}

	EulerAngle::EulerAngle(Angle pitch, Angle yaw, Angle roll) :
		Pitch(pitch), Yaw(yaw), Roll(roll)
	{
	}

	Pringle::Angle operator"" _deg(long double val)
	{
		return Pringle::Angle::Degrees((float)val);
	}
	
	Pringle::Angle operator"" _deg(unsigned long long val)
	{
		return Pringle::Angle::Degrees((float)val);
	}

	Pringle::Angle operator"" _rad(long double val)
	{
		return Pringle::Angle::Radians((float)val);
	}

	Pringle::Angle operator"" _rad(unsigned long long val)
	{
		return Pringle::Angle::Radians((float)val);
	}
}

