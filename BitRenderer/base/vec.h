/*
 * 向量类
 */
#ifndef VEC_H
#define VEC_H

#include "common.h"

template<int n>
class Vec
{
private:
	double e_[n];

public:
	double  operator[](const int& i) const { return e_[i]; }
	double& operator[](const int& i) { return e_[i]; }

	Vec() : e_{0} {}

	Vec(std::initializer_list<double> list)
	{
		for (auto it = list.begin(); it != list.end(); ++it)
		{
			e_[it - list.begin()] = *it;
		}
	}

	//用于设置imgui编辑的相机外参、颜色（imgui使用float）
	Vec(float t[3])
	{
		assert(n == 3);
		e_[0] = t[0];
		e_[1] = t[1];
		e_[2] = t[2];
	}

	Vec(double t0, double t1)
	{
		assert(n == 2);
		e_[0] = t0;
		e_[1] = t1;
	}

	Vec(double t0, double t1, double t2)
	{
		assert(n == 3);
		e_[0] = t0;
		e_[1] = t1;
		e_[2] = t2;
	}

	Vec(double t0, double t1, double t2, double t3)
	{
		assert(n == 3);
		e_[0] = t0;
		e_[1] = t1;
		e_[2] = t2;
		e_[3] = t3;
	}

	Vec(const Vec& v)
	{
		std::copy(std::begin(v.e_), std::end(v.e_), std::begin(e_));
	}

	Vec& operator=(const Vec& v)
	{
		std::copy(std::begin(v.e_), std::end(v.e_), std::begin(e_));
		return *this;
	}

	Vec operator-()
		const
	{
		Vec<n> v;
		for (int i = 0; i < n; ++i)
			v.e_[i] = -e_[i];
		return v;
	}

	Vec& operator+=(const Vec& v)
	{
		for (int i = 0; i < n; ++i)
			e_[i] += v.e_[i];
		return *this;
	}

	Vec& operator*=(double t)
	{
		for (int i = 0; i < n; ++i)
			e_[i] *= t;
		return *this;
	}

	Vec& operator/=(double t)
	{
		return *this *= 1 / t;
	}

	// 1范数
	double norm()
		const
	{
		return std::sqrt(norm2());
	}

	// 2范数
	double norm2()
		const
	{
		return dot(*this, *this);
	}

	// 归一化
	void normalize()
	{
		float l = this->norm();
		for (int i = 0; i < n; ++i)
			e_[i] /= l;;
		return;
	}

	bool near_zero()
		const
	{
		bool flag = true;
		for (int i = 0; i < n; ++i)
		{
			flag = flag && (fabs(e_[i]) < kEpsilon);
			if (!flag) return false;
		}
		return flag;
	}

	static Vec<n> random()
	{
		Vec<n> v;
		for (int i = 0; i < n; ++i)
			v.e_[i] = random_double();
		return v;
	}

	static Vec<n> random(double min, double max)
	{
		Vec<n> v;
		for (int i = 0; i < n; ++i)
			v.e_[i] = random_double(min, max);
		return v;
	}

/*
 * 访问成员 
 */
public:
	double x()
		const
	{
		return e_[0];
	}

	double& x()
	{
		return e_[0];
	}

	double y()
		const
	{
		return e_[1];
	}

	double& y()
	{
		return e_[1];
	}

	double z()
		const
	{
		return e_[2];
	}

	double& z()
	{
		return e_[2];
	}

	double w()
		const
	{
		return e_[3];
	}

	double& w()
	{
		return e_[3];
	}
	double r()
		const
	{
		return e_[0];
	}

	double& r()
	{
		return e_[0];
	}

	double g()
		const
	{
		return e_[1];
	}

	double& g()
	{
		return e_[1];
	}

	double b()
		const
	{
		return e_[2];
	}

	double& b()
	{
		return e_[2];
	}

	double a()
		const
	{
		return e_[3];
	}

	double& a()
	{
		return e_[3];
	}

	double u()
		const
	{
		return e_[0];
	}

	double& u()
	{
		return e_[0];
	}

	double v()
		const
	{
		return e_[1];
	}

	double& v()
	{
		return e_[1];
	}
};

using Vec2      = Vec<2>;
using Vec3      = Vec<3>;
using Vec4      = Vec<4>;
using Point3    = Vec3;
using Point4    = Vec4;
using Color3    = Vec3;
using Color4    = Vec4;
using Texcoord2 = Vec2;

template<int n>
std::ostream& operator<<(std::ostream& out, const Vec<n>& v)
{
	for (int i = 0; i < n; i++) out << v[i] << " ";
	return out;
}

template<int n>
Vec<n> operator+(const Vec<n>& lhs, const Vec<n>& rhs)
{
	Vec<n> ret = lhs;
	for (int i = n; i--; ret[i] += rhs[i]);
	return ret;
}

template<int n>
Vec<n> operator-(const Vec<n>& lhs, const Vec<n>& rhs)
{
	Vec<n> ret = lhs;
	for (int i = n; i--; ret[i] -= rhs[i]);
	return ret;
}

// 默认乘法为逐元素相乘而不是点积
template<int n>
Vec<n> operator*(const Vec<n>& lhs, const Vec<n>& rhs)
{
	Vec<n> ret;
	for (int i = n; i--; ret[i] = lhs[i] * rhs[i]);
	return ret;
}

template<int n>
double dot(const Vec<n>& lhs, const Vec<n>& rhs)
{
	double ret = 0;
	for (int i = n; i--; ret += lhs[i] * rhs[i]);
	return ret;
}

template<int n>
Vec<n> operator*(const double& rhs, const Vec<n>& lhs)
{
	Vec<n> ret = lhs;
	for (int i = n; i--; ret[i] *= rhs);
	return ret;
}

template<int n>
Vec<n> operator*(const Vec<n>& lhs, const double& rhs)
{
	Vec<n> ret = lhs;
	for (int i = n; i--; ret[i] *= rhs);
	return ret;
}

template<int n> 
Vec<n> operator/(const Vec<n>& lhs, const double& rhs)
{
	Vec<n> ret = lhs;
	for (int i = n; i--; ret[i] /= rhs);
	return ret;
}

template<int n1, int n2>
Vec<n1> embed(const Vec<n2>& v, double fill = 1)
{
	Vec<n1> ret;
	for (int i = n1; i--; ret[i] = (i < n2 ? v[i] : fill));
	return ret;
}

template<int n1, int n2>
Vec<n1> proj(const Vec<n2>& v)
{
	Vec<n1> ret;
	for (int i = n1; i--; ret[i] = v[i]);
	return ret;
}

inline Vec3 cross(const Vec3& u, const Vec3& v)
{
	return Vec3(
		u.y() * v.z() - u.z() * v.y(),
		u.z() * v.x() - u.x() * v.z(),
		u.x() * v.y() - u.y() * v.x());
}

inline Vec3 unit_vector(Vec3 v) 
{
	return v / v.norm();
}

inline Vec3 random_in_unit_sphere() 
{
	while (true)
	{
		auto p = Vec3::random(-1., 1.);
		if (p.norm2() < 1)
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

// 反射，v是入射光线方向，n是法线方向
inline Vec3 reflect(const Vec3& v, const Vec3& n)
{
	return v - 2 * dot(v, n) * n;
}

// 折射，uv入射光线方向，n是法线方向，etai_over_etat是折射率比值
// 参见RayTracingInOneWeekend 11.2
inline Vec3 refract(const Vec3& uv, const Vec3& n, double etai_over_etat) 
{
	auto cos_theta = fmin(dot(-uv, n), 1.0);
	Vec3 r_out_perp = etai_over_etat * (uv + cos_theta * n);
	Vec3 r_out_parallel = -sqrt(fabs(1.0 - r_out_perp.norm2())) * n;
	return r_out_perp + r_out_parallel;
}

// 模拟圆形透镜，随机采样
inline Vec3 random_in_unit_disk()
{
	while (true)
	{
		auto p = Vec3(random_double(-1, 1), random_double(-1, 1), 0);
		if (p.norm2() < 1)
			return p;
	}
}

// 半球cosine采样
inline Vec3 random_cosine_direction()
{
	auto r1 = random_double();
	auto r2 = random_double();

	auto phi = 2 * kPI * r1;
	auto x = cos(phi) * sqrt(r2);
	auto y = sin(phi) * sqrt(r2);
	auto z = sqrt(1 - r2);

	return Vec3(x, y, z);
}

#endif // !VEC_H
