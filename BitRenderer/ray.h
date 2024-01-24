/*
 * 光线类
 */
#ifndef RAY_H
#define RAY_H

#include "point.h"

class Ray
{
private:
	Point3 origin_;
	Vec3 direction_;
	double time_;

public:
	Ray() : origin_(), direction_(), time_(0) {}

	Ray(const Point3& origin, const Vec3& direction, double time = 0) 
		: origin_(origin), direction_(direction), time_(time) {};

public:
	// p = o + t * d
	Point3 at(double t)
		const
	{
		return origin_ + t * direction_;
	}

	Point3 get_origin()
		const
	{
		return origin_;
	}

	Vec3 get_direction() 
		const
	{
		return direction_;
	}

	double get_time() 
		const
	{
		return time_;
	}
};

#endif // !RAY_H
