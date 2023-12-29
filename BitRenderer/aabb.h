/*
* ��Χ��
* Axis-Aligned Bounding Boxe
*/
#ifndef AABB_H
#define AABB_H

#include "interval.h"
#include "point.h"
#include "ray.h"

class AABB
{
public:

    AABB() = default;
    AABB(const Interval& x, const Interval& y, const Interval& z)
        : x_(x), y_(y), z_(z) {}

    // ��Χ������ֵ��
    AABB(const Point3& a, const Point3& b)
    {
        x_ = Interval(fmin(a[0], b[0]), fmax(a[0], b[0]));
        y_ = Interval(fmin(a[1], b[1]), fmax(a[1], b[1]));
        z_ = Interval(fmin(a[2], b[2]), fmax(a[2], b[2]));
    }

    // ��������
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
        //    ray_t.set_min(fmax(t0, ray_t.get_min())); // ������СֵΪ����
        //    ray_t.set_max(fmin(t1, ray_t.get_max())); // �������ֵΪС��
        //    if (ray_t.get_max() <= ray_t.get_min()) // û���غ�˵�����߲����Χ���ཻ
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

private:

    Interval x_, y_, z_;
};

#endif // !AABB_H
