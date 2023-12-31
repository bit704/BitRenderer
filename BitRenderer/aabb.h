/*
* 包围盒
* Axis-Aligned Bounding Box
*/
#ifndef AABB_H
#define AABB_H

#include "interval.h"
#include "point.h"
#include "ray.h"

class AABB
{

    friend AABB operator+(const AABB& bbox, const Vec3& offset);
    friend AABB operator+(const Vec3& offset, const AABB& bbox);

public:

    AABB() = default;
    AABB(const Interval& x, const Interval& y, const Interval& z)
        : x_(x), y_(y), z_(z) {}

    // 包围盒两极值点
    AABB(const Point3& a, const Point3& b)
    {
        x_ = Interval(fmin(a[0], b[0]), fmax(a[0], b[0]));
        y_ = Interval(fmin(a[1], b[1]), fmax(a[1], b[1]));
        z_ = Interval(fmin(a[2], b[2]), fmax(a[2], b[2]));
    }

    // 并集运算
    AABB(const AABB& box0, const AABB& box1)
    {
        x_ = Interval(box0.x_, box1.x_);
        y_ = Interval(box0.y_, box1.y_);
        z_ = Interval(box0.z_, box1.z_);
    }

    const Interval& axis(int n) const 
    {
        if (n == 0) return x_;
        if (n == 1) return y_;
        return z_; // n == 2
    }

    bool hit(const Ray& r, Interval ray_t) const
    {
        //for (int a = 0; a < 3; ++a)
        //{
        //    double t0 = fmin((axis(a).get_min() - r.get_origin()[a]) / r.get_direction()[a],
        //        (axis(a).get_max() - r.get_origin()[a]) / r.get_direction()[a]);
        //    double t1 = fmax((axis(a).get_min() - r.get_origin()[a]) / r.get_direction()[a],
        //        (axis(a).get_max() - r.get_origin()[a]) / r.get_direction()[a]);
        //    ray_t.set_min(fmax(t0, ray_t.get_min())); // 更新最小值为大者
        //    ray_t.set_max(fmin(t1, ray_t.get_max())); // 更新最大值为小者
        //    if (ray_t.get_max() <= ray_t.get_min()) // 没有重合说明光线不与包围盒相交
        //        return false;
        //}

        // Andrew Kensler at Pixar
        for (int a = 0; a < 3; ++a) 
        {
            auto invD = 1 / r.get_direction()[a];
            auto orig = r.get_origin()[a];

            auto t0 = (axis(a).get_min() - orig) * invD;
            auto t1 = (axis(a).get_max() - orig) * invD;

            if (invD < 0)
                std::swap(t0, t1);

            if (t0 > ray_t.get_min()) ray_t.set_min(t0);
            if (t1 < ray_t.get_max()) ray_t.set_max(t1);

            if (ray_t.get_max() <= ray_t.get_min())
                return false;
        }
        return true;
    }

    // 避免过窄
    AABB pad()
    {
        double delta = 1e-4;

        Interval new_x = (x_.get_size() >= delta) ? x_ : x_.expand(delta);
        Interval new_y = (y_.get_size() >= delta) ? y_ : y_.expand(delta);
        Interval new_z = (z_.get_size() >= delta) ? z_ : z_.expand(delta);

        return AABB(new_x, new_y, new_z);
    }

    Interval x() const
    {
        return x_;
    }

    Interval y() const
    {
        return y_;
    }

    Interval z() const
    {
        return z_;
    }

private:

    Interval x_, y_, z_;
};

inline AABB operator+(const AABB& bbox, const Vec3& offset)
{
    return AABB(bbox.x_ + offset.x(), bbox.y_ + offset.y(), bbox.z_ + offset.z());
}

inline AABB operator+(const Vec3& offset, const AABB& bbox)
{
    return bbox + offset;
}

#endif // !AABB_H

