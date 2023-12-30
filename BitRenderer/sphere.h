/*
* 球类
*/
#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"
#include "vec3.h"

class Sphere : public Hittable
{
public:

    // 静态球
    Sphere(Point3 center, double radius, std::shared_ptr<Material> material) 
        : center_(center), radius_(radius), material_(material), is_moving_(false) 
    {
        auto rvec = Vec3(radius, radius, radius);
        bbox_ = AABB(center - rvec, center + rvec);
    }

    // 运动球
    Sphere(Point3 center, Point3 center_end, double radius, std::shared_ptr<Material> material)
        : center_(center), radius_(radius), material_(material), is_moving_(true)
    {
        auto rvec = Vec3(radius, radius, radius);
        AABB box1(center - rvec, center_end + rvec);
        AABB box2(center - rvec, center_end + rvec);
        bbox_ = AABB(box1, box2);

        center_move_vec_ = center_end - center;
    }

    bool hit(const Ray& r, Interval interval, HitRecord& rec) const override
    {
        Point3 now_center = is_moving_ ? get_center(r.get_time()) : center_;
        Vec3 oc = r.get_origin() - now_center;
        auto a = r.get_direction().length_squared();
        auto half_b = dot(oc, r.get_direction());
        auto c = oc.length_squared() - radius_ * radius_;

        auto discriminant = half_b * half_b - a * c;
        if (discriminant < 0) 
            return false;
        auto sqrtd = std::sqrt(discriminant);

        auto root = (-half_b - sqrtd) / a; // 更近的根
        if (root <= interval.get_min() || interval.get_max() <= root)
        {
            root = (-half_b + sqrtd) / a; // 更远的根
            if (root <= interval.get_min() || interval.get_max() <= root)
                return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        Vec3 outward_normal = (rec.p - center_) / radius_; // 单位化
        rec.set_face_normal(r, outward_normal);
        get_sphere_uv(outward_normal, rec.u, rec.v);
        rec.material = material_;

        return true;
    }

    AABB get_bbox() const override
    { 
        return bbox_;
    }

    // 参见RayTracingTheNextWeek 4.4
    // p: 球上一点
    // u: returned value [0,1] of angle around the Y axis from X=-1.
    // v: returned value [0,1] of angle from Y=-1 to Y=+1.
    static void get_sphere_uv(const Point3& p, double& u, double& v)
    {
        //     <1 0 0> yields <0.50 0.50>       <-1  0  0> yields <0.00 0.50>
        //     <0 1 0> yields <0.50 1.00>       < 0 -1  0> yields <0.50 0.00>
        //     <0 0 1> yields <0.25 0.50>       < 0  0 -1> yields <0.75 0.50>
        double theta = acos(-p.y());
        double phi = atan2(-p.z(), p.x()) + kPI;
        u = phi / (2 * kPI);
        v = theta / kPI;
    }

private:

    Point3 center_;
    Vec3 center_move_vec_; // 球移动距离
    bool is_moving_; //球是否在移动
    AABB bbox_;

    double radius_;
    std::shared_ptr<Material> material_;

    // 获取t时刻的球心位置
    Point3 get_center(double t) const
    {
        return center_ + t * center_move_vec_;
    }

};

#endif // !SPHERE_H
