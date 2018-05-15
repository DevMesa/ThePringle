#include "Vector.hpp"
#include "QAngle.hpp"

namespace Pringle
{
	Vector Vector::Zero()  { return Vector(0, 0, 0); }
	Vector Vector::Up()    { return Vector(0, 0, 1); }
	Vector Vector::Down()  { return Vector(0, 0, -1); }
	Vector Vector::North() { return Vector(0, 1, 0); }
	Vector Vector::South() { return Vector(0, -1, 0); }
	Vector Vector::East()  { return Vector(1, 0, 0); }
	Vector Vector::West()  { return Vector(-1, 0, 0); }

	Vector::Vector() :
		Vector(Vector::Zero())
	{}
	Vector::Vector(float x, float y, float z) :
		X(x), Y(y), Z(z)
	{}
	Vector::Vector(const Blam::Math::RealVector3D& other) :
		Vector(other.I, other.J, other.K)
	{}
	Vector::Vector(const D3DXVECTOR3 & other) :
		Vector(other.x, other.y, other.z)
	{}

	Vector::Vector(Angle pitch, Angle yaw) :
		X(Cos(yaw)), Y(Sin(yaw)), Z(Sin(pitch))
	{
	}

	float Vector::LengthSequared() const
	{
		return this->X * this->X + this->Y * this->Y + this->Z * this->Z;
	}

	float Vector::Length() const
	{
		return std::sqrt(this->LengthSequared());
	}

	Vector& Vector::Normalize()
	{
		float len = this->Length();

		if (len == 0.0f)
			return *this;

		this->X /= len;
		this->Y /= len;
		this->Z /= len;

		return *this;
	}

	Vector Vector::Normal() const
	{
		return Vector(*this).Normalize();
	}

	Vector& Vector::Crossize(const Vector& other)
	{
		Vector old = *this;

		this->X = old.Y * other.Z - old.Z * other.Y;
		this->Y = old.Z * other.X - old.X * other.Z;
		this->Z = old.X * other.Y - old.Y * other.X;

		return *this;
	}

	Vector Vector::Cross(const Vector& other) const
	{
		return Vector(*this).Crossize(other);
	}

	float Vector::Dot(const Vector& other) const
	{
		return this->X * other.X + this->Y * other.Y + this->Z * other.Z;
	}

	Vector Vector::Parallel(const Vector& axis) const
	{
		return axis * this->Dot(axis);
	}

	Vector Vector::Perp(const Vector& axis) const
	{
		return *this - this->Dot(axis);
	}

	Vector Vector::RotateAroundAxis(const Vector& axis, Angle angle)
	{
		float cos_theta = Cos(angle);
		float sin_theta = Sin(angle);

		return this->Perp(axis) * cos_theta + axis.Cross(*this) * sin_theta + this->Parallel(axis);
	}

	Vector Vector::ProjectOnPlane(const Vector& normal)
	{
		return *this - normal * this->Dot(normal);
	}

	EulerAngles Vector::Angles() const
	{
		auto norm = this->Normal();
		return EulerAngles(Asin(norm.Z), Atan2(norm.Y, norm.X), 0_deg);
	}

	bool Vector::ToScreen(const Vector& cam_pos, const QAngle& cam_ang, Angle fov, float width, float height, float& out_x, float& out_y) const
	{
		Vector up = cam_ang.Up();
		Vector right = cam_ang.Right();
		Vector forward = cam_ang.Forward();

		Vector dir = cam_pos - *this;

		float dot = forward.Dot(dir);
		
		if (dot <= 0.0f)
			return false;

		float d = 4.0f * height / (6.0f * Tan(fov * 0.5f));
		Vector proj = dir * (d / dot);

		out_x = 0.5f * width + right.Dot(proj);
		out_y = 0.5f * height + up.Dot(proj);

		return 
			0.0f <= out_x && out_x <= width &&
			0.0f <= out_y && out_y <= height;
	}
	Vector Vector::Right() const
	{
		return Vector(this->X, this->Y, 0.0f).Cross(Vector::Up());
	}
}