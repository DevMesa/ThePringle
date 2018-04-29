#pragma once

#ifndef PRINGLE_QUATERNIONH
#define PRINGLE_QUATERNIONH

#include <d3d9.h>
#include "../Blam/Math/RealVector3D.hpp"

#include "EulerAngle.hpp"
#include "Vector.hpp"

namespace Pringle
{
	struct Vector;
	struct Quaternion
	{
		static Quaternion Identity();;

		Vector Xyz;
		float  W;

		Quaternion();
		Quaternion(const Vector& xyz, float w);
		Quaternion(float  x, float y, float z, float w) : Xyz(x, y, z), W(w) {}
		Quaternion(const Vector& axis, Angle angle);

		Quaternion& Normalize();
		Quaternion Normal() const;
	};

	Vector operator*(const Vector& left, const Quaternion& right);
	Quaternion operator*(const Quaternion& left, const Quaternion& right);
}

#endif // !PRINGLE_QUATERNIONH
