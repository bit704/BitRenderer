/*
* π‚œﬂ¿‡
*/
#ifndef RAY_H
#define RAY_H

#include "point.h"

class Ray
{
public:

	Ray(const Point3& origin, const Vec3& direction) : origin_(origin), direction_(direction) {};

	Point3 get_origin() const
	{
		return origin_;
	}

	Vec3 get_direction() const
	{
		return direction_;
	}
	
	// p = o + t * d
	Point3 at(double t) const
	{
		return origin_ + t * direction_;
	}

private:

	Point3 origin_;
	Vec3 direction_;
};

#endif // !RAY_H
