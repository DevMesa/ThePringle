#include "Quaternion.hpp"

namespace Pringle
{
	Quaternion Quaternion::Identity() { return Quaternion(Vector::Zero(), 1); }

	Quaternion::Quaternion() : Quaternion(Quaternion::Identity()) {}

	Quaternion::Quaternion(const Vector & xyz, float w) : Xyz(xyz), W(w) {}

	Quaternion::Quaternion(const Vector & axis, Angle angle) : Xyz(0, 0, 0), W(1)
	{
		if (axis.LengthSequared() == 0.0f)
			return;

		this->Xyz = axis.Normal() * Sin(angle);
		this->W = Cos(angle);
		this->Normalize();
	}

	Quaternion& Quaternion::Normalize()
	{
		float len = std::sqrt(this->Xyz.LengthSequared() + this->W * this->W);
		this->Xyz /= len;
		this->W /= len;
		return *this;
	}
	Quaternion Quaternion::Normal() const
	{
		return Quaternion(*this).Normalize();
	}

	Vector operator*(const Vector& left, const Quaternion& right)
	{
		const Vector& xyz = right.Xyz;
		Vector tmp1 = xyz.Cross(left);
		Vector tmp2 = left * right.W;
		Vector tmp3 = tmp1 + tmp2;
		Vector tmp4 = xyz.Cross(tmp3);
		Vector tmp5 = xyz * 2.0f;
		Vector ret_test = left + tmp5;

		return left + xyz.Cross((xyz.Cross(left)) + (left * right.W)) * 2.0f;
	}

	Quaternion operator*(const Quaternion& left, const Quaternion& right)
	{
		return Quaternion(
			left.Xyz * right.W + right.Xyz * left.W + (left.Xyz.Cross(right.Xyz)),
			left.W * right.W - (left.Xyz.Dot(right.Xyz))
		);
	}
}