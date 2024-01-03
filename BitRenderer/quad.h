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
        area_ = n.length();
        normal_ = unit_vector(n);
        D_ = dot(normal_, Q);
        w_ = n / dot(n, n);
        set_bbox();
    }

    double pdf_value(const Point3& origin, const Vec3& v) const override
    {
        HitRecord rec;
        if (!this->hit(Ray(origin, v), Interval(1e-3, kInfinitDouble), rec))
            return 0;

        auto distance_squared = rec.t * rec.t * v.length_squared();
        auto cosine = fabs(dot(v, rec.normal) / v.length());

        return distance_squared / (cosine * area_);
    }

    Vec3 random(const Point3& origin) const override
    {
        auto p = Q_ + (random_double() * u_) + (random_double() * v_);
        return p - origin;
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
    double area_;

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

// 两点生成立方体
inline std::shared_ptr<HittableList> construct_box(const Point3& a, const Point3& b, std::shared_ptr<Material> mat)
{
    auto sides = std::make_shared<HittableList>();

    // 构造两极值点
    auto min = Point3(fmin(a.x(), b.x()), fmin(a.y(), b.y()), fmin(a.z(), b.z()));
    auto max = Point3(fmax(a.x(), b.x()), fmax(a.y(), b.y()), fmax(a.z(), b.z()));

    auto dx = Vec3(max.x() - min.x(), 0, 0);
    auto dy = Vec3(0, max.y() - min.y(), 0);
    auto dz = Vec3(0, 0, max.z() - min.z());

    sides->add(std::make_shared<Quad>(Point3(min.x(), min.y(), max.z()), dx, dy, mat)); // 前
    sides->add(std::make_shared<Quad>(Point3(max.x(), min.y(), max.z()), -dz, dy, mat)); // 右
    sides->add(std::make_shared<Quad>(Point3(max.x(), min.y(), min.z()), -dx, dy, mat)); // 后
    sides->add(std::make_shared<Quad>(Point3(min.x(), min.y(), min.z()), dz, dy, mat)); // 左
    sides->add(std::make_shared<Quad>(Point3(min.x(), max.y(), max.z()), dx, -dz, mat)); // 前
    sides->add(std::make_shared<Quad>(Point3(min.x(), min.y(), min.z()), dx, dz, mat)); // 底

    return sides;
}

#endif // !QUAD_H