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

		float R;
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
		explicit Angle(float radians);
	};

	inline float Sin(Angle ang) { return std::sin(ang.R); }
	inline Angle Asin(float x) { return Angle::Radians(std::asin(x)); }
	inline float Cos(Angle ang) { return std::cos(ang.R); }
	inline Angle Acos(float x) { return Angle::Radians(std::acos(x)); }
	inline float Tan(Angle ang) { return std::tan(ang.R); }
	inline Angle Atan(float x) { return Angle::Radians(std::atan(x)); }
	inline Angle Atan2(float x, float y) { return Angle::Radians(std::atan2(x, y)); }

	struct EulerAngle
	{
		Angle Pitch, Yaw, Roll;
		EulerAngle();
		EulerAngle(Angle pitch, Angle yaw, Angle roll);
	};

	Pringle::Angle operator"" _deg(long double val);
	Pringle::Angle operator"" _deg(unsigned long long val);
	Pringle::Angle operator"" _rad(long double val);
	Pringle::Angle operator"" _rad(unsigned long long val);
}




#endif // !PRINGLE_ANGLEH
