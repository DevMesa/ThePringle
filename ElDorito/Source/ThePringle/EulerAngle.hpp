#pragma once

#ifndef PRINGLE_ANGLEH
#define PRINGLE_ANGLEH

#include <cmath>

#include "Vector.hpp"

namespace Pringle
{
	struct Angle
	{
		static Angle Degrees(float deg);
		static Angle Radians(float rad);

		float AsRadians();
		float AsDegrees();

		explicit Angle();
		
		inline Angle operator/(float scaler) const
		{ return Angle(this->R / scaler); }

		inline Angle operator*(float scaler) const
		{ return Angle(this->R * scaler); }

		inline Angle operator+(Angle other) const
		{ return Angle(this->R + other.R); }

		inline Angle operator-(Angle other) const
		{ return Angle(this->R - other.R); }

		inline Angle operator-() const
		{ return Angle(-this->R); }
	protected:
		float R;
		explicit Angle(float radians);
	};

	inline float Sin(Angle ang) { return std::sin(ang.AsRadians()); }
	inline Angle Asin(float x) { return Angle::Radians(std::asin(x)); }
	inline float Cos(Angle ang) { return std::cos(ang.AsRadians()); }
	inline Angle Acos(float x) { return Angle::Radians(std::acos(x)); }
	inline float Tan(Angle ang) { return std::tan(ang.AsRadians()); }
	inline Angle Atan(float x) { return Angle::Radians(std::atan(x)); }
	inline Angle Atan2(float y, float x) { return Angle::Radians(std::atan2(y, x)); }

	struct EulerAngles
	{
		Angle Pitch, Yaw, Roll;
		EulerAngles();
		EulerAngles(Angle pitch, Angle yaw, Angle roll);
	};

	Pringle::Angle operator"" _deg(long double val);
	Pringle::Angle operator"" _deg(unsigned long long val);
	Pringle::Angle operator"" _rad(long double val);
	Pringle::Angle operator"" _rad(unsigned long long val);
}




#endif // !PRINGLE_ANGLEH
