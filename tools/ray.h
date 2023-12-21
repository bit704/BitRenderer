/*
* π‚œﬂ¿‡
*/
#ifndef RAY_H
#define RAY_H

#include "point.h"

class Ray
{
public:

	Ray(const Point3& get_origin, const Vec3& get_direction) : origin(get_origin), direction(get_direction) {};

	Point3 get_origin() const
	{
		return origin;
	}

	Vec3 get_direction() const
	{
		return direction;
	}
	
	// p = o + t * d
	Point3 at(double t) const
	{
		return origin + t * direction;
	}

private:

	Point3 origin;
	Vec3 direction;
};

#endif // !RAY_H
