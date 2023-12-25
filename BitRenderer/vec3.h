/*
* 长度为3的向量类
*/
#ifndef Vec3_H
#define Vec3_H

#include <cmath>
#include <iostream>
#include "common.h"

class Vec3
{
public:
	double e_[3];

	Vec3() : e_{0,0,0} {}
	Vec3(double e0, double e1, double e2) : e_{e0,e1,e2} {}

	double x() const { return e_[0]; }
	double y() const { return e_[1]; }
	double z() const { return e_[2]; }

	Vec3 operator-() const { return Vec3(-e_[0], -e_[1], -e_[2]); }
	double operator[](int i) const { return e_[i]; }
	double& operator[](int i) { return e_[i]; }

	Vec3& operator+=(const Vec3& v)
	{
		e_[0] += v.e_[0];
		e_[1] += v.e_[1];
		e_[2] += v.e_[2];
		return *this;
	}

	Vec3& operator*=(double t)
	{
		e_[0] *= t;
		e_[1] *= t;
		e_[2] *= t;
		return *this;
	}

	Vec3& operator/=(double t)
	{
		return *this *= 1 / t;
	}

	double length_squared() const
	{
		return e_[0] * e_[0] + e_[1] * e_[1] + e_[2] * e_[2];
	}

	double length() const
	{
		return std::sqrt(length_squared());
	}

	bool near_zero() const
	{
		double s = 1e-8;
		return (fabs(e_[0]) < s) && (fabs(e_[1]) < s) && (fabs(e_[2]) < s);
	}

	static Vec3 random() 
	{
		return Vec3(random_double(), random_double(), random_double());
	}

	static Vec3 random(double min, double max) 
	{
		return Vec3(random_double(min, max), random_double(min, max), random_double(min, max));
	}

};

inline std::ostream& operator<<(std::ostream& out, const Vec3& v)
{
	return out << v.e_[0] << ' ' << v.e_[1] << ' ' << v.e_[2];
}

inline Vec3 operator+(const Vec3& u, const Vec3& v)
{
	return Vec3(u.e_[0] + v.e_[0], u.e_[1] + v.e_[1], u.e_[2] + v.e_[2]);
}

inline Vec3 operator-(const Vec3& u, const Vec3& v)
{
	return Vec3(u.e_[0] - v.e_[0], u.e_[1] - v.e_[1], u.e_[2] - v.e_[2]);
}

inline Vec3 operator*(const Vec3& u, const Vec3& v)
{
	return Vec3(u.e_[0] * v.e_[0], u.e_[1] * v.e_[1], u.e_[2] * v.e_[2]);
}

inline Vec3 operator*(double t, const Vec3& v)
{
	return Vec3(t * v.e_[0], t * v.e_[1], t * v.e_[2]);
}

inline Vec3 operator*(const Vec3& v, double t) 
{
	return t * v;
}

inline Vec3 operator/(Vec3 v, double t) 
{
	return (1 / t) * v;
}

inline double dot(const Vec3& u, const Vec3& v) 
{
	return u.e_[0] * v.e_[0]
		 + u.e_[1] * v.e_[1]
		 + u.e_[2] * v.e_[2];
}

inline Vec3 cross(const Vec3& u, const Vec3& v)
{
	return Vec3(u.e_[1] * v.e_[2] - u.e_[2] * v.e_[1],
		u.e_[2] * v.e_[0] - u.e_[0] * v.e_[2],
		u.e_[0] * v.e_[1] - u.e_[1] * v.e_[0]);
}

inline Vec3 unit_vector(Vec3 v) 
{
	return v / v.length();
}

inline Vec3 random_in_unit_sphere() 
{
	while (true)
	{
		auto p = Vec3::random(-1., 1.);
		if (p.length_squared() < 1)
			return p;
	}
}

// 单位球上采样单位向量
inline Vec3 random_unit_vector()
{
	return unit_vector(random_in_unit_sphere());
}

inline Vec3 random_on_hemisphere(const Vec3& normal) 
{
	Vec3 on_unit_sphere = random_unit_vector();
	// 在法向量所在的半球
	if (dot(on_unit_sphere, normal) > 0.) 
		return on_unit_sphere;
	else
		return -on_unit_sphere;
}

// 反射，v是入射光线方向，n是反射方向
inline Vec3 reflect(const Vec3& v, const Vec3& n)
{
	return v - 2 * dot(v, n) * n;
}

#endif // !Vec3_H
