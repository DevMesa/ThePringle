
#include "QAngle.hpp"

namespace Pringle
{
	QAngle QAngle::Identity()
	{
		return QAngle();
	}

	QAngle::QAngle() :
		Quat(Quaternion::Identity())
	{
	}
	QAngle::QAngle(const Quaternion& quat) :
		Quat(quat)
	{
	}

	QAngle::QAngle(Angle pitch, Angle yaw, Angle roll)
	{
		this->Quat =
			Quaternion(Vector::North(), roll) *
			Quaternion(Vector::Down(), yaw) *
			Quaternion(Vector::East(), pitch);
	}
	QAngle::QAngle(Vector direction, Angle roll)
	{
		throw "not imp";
	}
	QAngle QAngle::RotateAroundAxis(const Vector & axis, Angle ang)
	{
		return QAngle(axis, ang).Quat * this->Quat;
	}
	
	Vector QAngle::Forward() const
	{
		return Vector::North() * *this;
	}

	Vector QAngle::Up() const
	{
		return Vector::Up() * *this;
	}

	Vector QAngle::Right() const
	{
		return Vector::East() * *this;
	}

	Vector QAngle::Down() const
	{
		return -this->Up();
	}

	Vector QAngle::Left() const
	{
		return -this->Right();
	}

	Vector QAngle::Backward() const
	{
		return -this->Forward();
	}

}