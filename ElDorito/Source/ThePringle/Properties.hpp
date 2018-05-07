#pragma once
#ifndef PRINGLE_PROPERTIES
#define PRINGLE_PROPERTIES

namespace Pringle
{
	template<typename T>
	class Property
	{
	private:
		std::function<void(T& stored, const T& value)> Setter;
		std::function<T(T& stored)> Getter;
		T Value;
	public:
		Property() :
			Setter([](T& stored, const T& value) { stored = value; }),
			Getter([](T& stored)->T {return stored; })
		{
		}

		Property(const Property<T>&& other)
		{
			*this = (T)other;
		}

		Property(std::function<void(T& stored, const T& value)> setter, std::function<T(T& stored)> getter) :
			Setter(setter),
			Getter(getter)
		{
		}

		T& operator =(const T& value)
		{
			this->Setter(this->Value, value);
			return value;
		}
		operator const T() const
		{
			return this->Getter(this->Value);
		}
	};
}

#endif