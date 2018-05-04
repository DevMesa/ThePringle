#pragma once

#ifndef PRINGLE_QANGLEH
#define PRINGLE_QANGLEH

#include <cmath>

#include "Vector.hpp"
#include "EulerAngle.hpp"
#include "Quaternion.hpp"

namespace Pringle
{
	struct QAngle
	{
		static QAngle Identity();

		Quaternion Quat;

		QAngle();
		QAngle(const Quaternion& quat);
		QAngle(Angle pitch, Angle yaw, Angle roll);
		QAngle(Vector direction, Angle roll);

		QAngle RotateAroundAxis(const Vector& axis, Angle ang);

		Vector Forward() const;
		Vector Up() const;
		Vector Right() const;
		Vector Down() const;
		Vector Left() const;
		Vector Backward() const;
	};

	inline Vector operator*(const Vector& left, const QAngle& right)
	{
		return left * right.Quat;
	}
}

#endif // !PRINGLE_QANGLEH
