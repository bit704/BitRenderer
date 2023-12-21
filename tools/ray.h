/*
* ������
*/
#ifndef RAY_H
#define RAY_H

#include "vec3.h"

class Ray
{
public:

	Ray(const point3& origin, const Vec3& direction) : ori(origin), dir(direction) {};

	point3 origin() const
	{
		return ori;
	}

	Vec3 direction() const
	{
		return dir;
	}
	
	// p = o + t * d
	point3 at(double t) const
	{
		return ori + t * dir;
	}

private:

	point3 ori;
	Vec3 dir;
};

#endif // !RAY_H