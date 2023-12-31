/*
* 平行四边形类
*/
#ifndef QUAD_H
#define QUAD_H

#include "common.h"
#include "hittable.h"

class Quad : public Hittable
{
public:

    Quad(const Point3& Q, const Vec3& u, const Vec3& v, std::shared_ptr<Material> m)
        : Q_(Q), u_(u), v_(v), material_(m)
    {
        Vec3 n = cross(u, v);
        normal_ = unit_vector(n);
        D_ = dot(normal_, Q);
        w_ = n / dot(n, n);
        set_bbox();
    }

    virtual void set_bbox()
    {
        bbox_ = AABB(Q_, Q_ + u_ + v_).pad();
    }

    AABB get_bbox() const override 
    { 
        return bbox_;
    }

    bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const override 
    {
        auto denom = dot(normal_, r.get_direction());

        // 光线与平面法线垂直
        if (fabs(denom) < kEpsilon)
            return false;

        auto t = (D_ - dot(normal_, r.get_origin())) / denom;

        // 超出光线范围
        if (!ray_t.contains(t))
            return false;

        Point3 intersection = r.at(t); // 交点

        Vec3 planar_hitpt_vector = intersection - Q_;
        auto alpha = dot(w_, cross(planar_hitpt_vector, v_));
        auto beta = dot(w_, cross(u_, planar_hitpt_vector));

        if (!is_interior(alpha, beta, rec))
            return false;

        rec.t = t;
        rec.p = intersection;
        rec.material = material_;
        rec.set_face_normal(r, normal_);

        return true;
    }

private:

    Point3 Q_;
    Vec3 u_, v_;
    std::shared_ptr<Material> material_;
    AABB bbox_;
    Vec3 normal_;
    double D_; // Ax+By+Cz=D
    Vec3 w_; // 参见RayTracingTheNextWeek 6.4

    // 判断平行四边形所在平面上一点是否在平行四边形内并设定uv坐标
    virtual bool is_interior(double a, double b, HitRecord& rec) const
    {
        if ((a < 0) || (1 < a) || (b < 0) || (1 < b))
            return false;
        rec.u = a;
        rec.v = b;
        return true;
    }
};

#endif // !QUAD_H