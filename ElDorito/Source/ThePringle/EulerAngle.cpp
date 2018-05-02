
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

	float Angle::AsRadians()
	{
		return this->R;
	}

	float Angle::AsDegrees()
	{
		return this->R / pi<float> * 180.0f;
	}

	Angle::Angle() :
		R(0)
	{
	}

	Angle::Angle(float radians) :
		R(radians)
	{
	}

	EulerAngles::EulerAngles() :
		EulerAngles(0_deg, 0_deg, 0_deg)
	{
	}

	EulerAngles::EulerAngles(Angle pitch, Angle yaw, Angle roll) :
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

