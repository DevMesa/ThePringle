#pragma once

#ifndef PRINGLE_VECTORH
#define PRINGLE_VECTORH

#include <d3dx9math.h>

#include "../Blam/Math/RealVector3D.hpp"

#include <cmath>

#include "EulerAngle.hpp"

namespace Pringle
{
	struct Angle;
	struct QAngle;
	struct Vector
	{
		static Vector Zero();
		static Vector Up();
		static Vector Down();
		static Vector North();
		static Vector South();
		static Vector East();
		static Vector West();

		float  X, Y, Z;

		Vector();
		Vector(float  x, float y, float z);
		Vector(const Blam::Math::RealVector3D& other);
		Vector(const D3DXVECTOR3& other);

		operator Blam::Math::RealVector3D() const
		{ return Blam::Math::RealVector3D(this->X, this->Y, this->Z); }
		operator D3DXVECTOR3() const
		{ return D3DXVECTOR3(this->X, this->Y, this->Z); }

		float LengthSequared() const;
		float Length() const;

		Vector& Normalize();
		Vector Normal() const;

		Vector& Crossize(const Vector& other);
		Vector Cross(const Vector& other) const;
		float Dot(const Vector& other) const;
		Vector Parallel(const Vector& axis) const;
		Vector Perp(const Vector& axis) const;
		Vector RotateAroundAxis(const Vector& axis, Angle angle);

		bool ToScreen(const Vector& cam_pos, const QAngle& cam_ang, Angle fov, float width, float height, float& out_x, float& out_y) const;

		// generate the various operations
#define BINOP(OP) \
		inline Vector operator##OP(const Vector& other) const \
		{ return Vector(this->X OP other.X, this->Y OP other.Y, this->Z OP other.Z); } \
		inline Vector operator##OP(float scaler) const \
		{ return Vector(this->X OP scaler, this->Y OP scaler, this->Z OP scaler); } \
		inline Vector operator##OP##=(const Vector& other) \
		{ \
			this->X OP##= other.X; \
			this->Y OP##= other.Y; \
			this->Z OP##= other.Z; \
			return *this; \
		} \
		inline Vector operator##OP##=(float scaler) \
		{ \
			this->X OP##= scaler; \
			this->Y OP##= scaler; \
			this->Z OP##= scaler; \
			return *this;\
		}

		BINOP(/);
		BINOP(*);
		BINOP(+);
		BINOP(-);

		inline Vector operator-() const
		{
			return Vector(-this->X, -this->Y, -this->Z);
		}
#undef BINOP
	};
}


#endif // !PRINGLE_VECTORH
