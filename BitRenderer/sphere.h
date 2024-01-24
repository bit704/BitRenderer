/*
 * 球类
 */
#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"
#include "vec3.h"
#include "onb.h"

class Sphere : public Hittable
{
private:
    Point3 center_;
    Vec3 center_move_vec_; // 球移动距离
    bool is_moving_; //球是否在移动
    AABB bbox_;
    double radius_;
    std::shared_ptr<Material> material_;

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

    Sphere(const Sphere&) = delete;
    Sphere& operator=(const Sphere&) = delete;

    Sphere(Sphere&&) = delete;
    Sphere& operator=(Sphere&&) = delete;

public:
    bool hit(const Ray& r, Interval interval, HitRecord& rec) 
        const override
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
        if (!interval.surrounds(root))
        {
            root = (-half_b + sqrtd) / a; // 更远的根
            if (!interval.surrounds(root))
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
    // 根据球上一点三维坐标得到其uv纹理坐标
    // p: 球上一点
    // u: returned value [0,1] of angle around the Y axis from X=-1.
    // v: returned value [0,1] of angle from Y=-1 to Y=+1.
    static void get_sphere_uv(const Point3& p, double& u, double& v)
    {
        //     <1 0 0> yields <0.50 0.50>       <-1  0  0> yields <0.00 0.50>
        //     <0 1 0> yields <0.50 1.00>       < 0 -1  0> yields <0.50 0.00>
        //     <0 0 1> yields <0.25 0.50>       < 0  0 -1> yields <0.75 0.50>
        auto phi = atan2(-p.z(), p.x()) ;
        u = (phi + kPI) / (2 * kPI);
        auto theta = acos(-p.y());
        v = theta / kPI;
    }

    double pdf_value(const Point3& o, const Vec3& v) const override
    {
        // 仅对静态球有效
        HitRecord rec;
        if (!this->hit(Ray(o, v), Interval(0.001, kInfinitDouble), rec))
            return 0;

        auto cos_theta_max = std::sqrt(1 - radius_ * radius_ / (center_ - o).length_squared());
        auto solid_angle = 2 * kPI * (1 - cos_theta_max);

        return  1 / solid_angle;
    }

    Vec3 random(const Point3& o) const override
    {
        Vec3 direction = center_ - o;
        auto distance_squared = direction.length_squared();
        ONB uvw;
        uvw.build_from_w(direction);
        return uvw.local(random_to_sphere(radius_, distance_squared));
    }

private:
    // 获取t时刻的球心位置
    Point3 get_center(double t) const
    {
        return center_ + t * center_move_vec_;
    }

    static Vec3 random_to_sphere(double radius, double distance_squared)
    {
        auto r1 = random_double();
        auto r2 = random_double();
        auto z = 1 + r2 * (sqrt(1 - radius * radius / distance_squared) - 1);

        auto phi = 2 * kPI * r1;
        auto x = cos(phi) * sqrt(1 - z * z);
        auto y = sin(phi) * sqrt(1 - z * z);

        return Vec3(x, y, z);
    }
};

#endif // !SPHERE_H
