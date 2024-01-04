/*
* 光线可击中物体类
*/
#ifndef HITTABLE_H
#define HITTABLE_H

#include "aabb.h"

// 前向声明，避免头文件循环引用
class Material;

struct HitRecord
{
    Point3 p;
    Vec3 normal;
    std::shared_ptr<Material> material;
    double t;
    bool front_face;
    double u, v;

    // 确保法线面向观察者
    // outward_normal为从球心指向光线击中点方向的法线
    void set_face_normal(const Ray& r, const Vec3& outward_normal) 
    {
        front_face = dot(r.get_direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

// 纯虚类
class Hittable
{
public:

    virtual ~Hittable() = default;
    virtual bool hit(const Ray& r, Interval interval, HitRecord& rec) const = 0;
    virtual AABB get_bbox() const = 0;

    virtual double pdf_value(const Point3& o, const Vec3& v) const
    {
        return 0.;
    }

    virtual Vec3 random(const Vec3& o) const
    {
        return Vec3(1, 0, 0);
    }
};

// 参见 RayTracingTheNextWeek 8.1
// 将对物体的平移等效为对ray的
class Translate : public Hittable
{
public:

    Translate(std::shared_ptr<Hittable> p, const Vec3& displacement)
        : object_(p), offset_(displacement)
    {
        bbox_ = object_->get_bbox() + offset_;
    }

    bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const override
    {
        // 根据offset反向移动光线
        Ray offset_r(r.get_origin() - offset_, r.get_direction(), r.get_time());

        if (!object_->hit(offset_r, ray_t, rec))
            return false;

        // 将交点根据offset正向移动回去
        rec.p += offset_;
        return true;
    }

    AABB get_bbox() const override
    { 
        return bbox_;
    }

private:

    std::shared_ptr<Hittable> object_;
    Vec3 offset_;
    AABB bbox_;
};

// 将对物体的绕Y轴转动等效为对ray的
class RotateY : public Hittable
{
public:
    RotateY(std::shared_ptr<Hittable> p, double angle) : object_(p)
    {
        auto radians = degrees_to_radians(angle);
        sin_theta_ = sin(radians);
        cos_theta_ = cos(radians);
        bbox_ = object_->get_bbox();

        Point3 min_p(kInfinitDouble, kInfinitDouble, kInfinitDouble);
        Point3 max_p(-kInfinitDouble, -kInfinitDouble, -kInfinitDouble);

        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                for (int k = 0; k < 2; k++)
                {
                    auto x = i * bbox_.x().get_max() + (1 - i) * bbox_.x().get_min();
                    auto y = j * bbox_.y().get_max() + (1 - j) * bbox_.y().get_min();
                    auto z = k * bbox_.z().get_max() + (1 - k) * bbox_.z().get_min();

                    auto newx = cos_theta_ * x + sin_theta_ * z;
                    auto newz = -sin_theta_ * x + cos_theta_ * z;

                    Vec3 tester(newx, y, newz);

                    for (int c = 0; c < 3; c++)
                    {
                        min_p[c] = fmin(min_p[c], tester[c]);
                        max_p[c] = fmax(max_p[c], tester[c]);
                    }
                }
            }
        }

        bbox_ = AABB(min_p, max_p);
    }

    bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const override
    {
        // 将ray从世界空间转换到模型空间
        auto origin = r.get_origin();
        auto direction = r.get_direction();

        origin[0] = cos_theta_ * r.get_origin()[0] - sin_theta_ * r.get_origin()[2];
        origin[2] = sin_theta_ * r.get_origin()[0] + cos_theta_ * r.get_origin()[2];

        direction[0] = cos_theta_ * r.get_direction()[0] - sin_theta_ * r.get_direction()[2];
        direction[2] = sin_theta_ * r.get_direction()[0] + cos_theta_ * r.get_direction()[2];

        Ray rotated_r(origin, direction, r.get_time());

        if (!object_->hit(rotated_r, ray_t, rec))
            return false;

        auto p = rec.p;
        p[0] = cos_theta_ * rec.p[0] + sin_theta_ * rec.p[2];
        p[2] = -sin_theta_ * rec.p[0] + cos_theta_ * rec.p[2];

        auto normal = rec.normal;
        normal[0] = cos_theta_ * rec.normal[0] + sin_theta_ * rec.normal[2];
        normal[2] = -sin_theta_ * rec.normal[0] + cos_theta_ * rec.normal[2];

        rec.p = p;
        rec.normal = normal;

        return true;
    }

    virtual AABB get_bbox() const 
    {
        return bbox_;
    }

private:

    std::shared_ptr<Hittable> object_;
    double sin_theta_;
    double cos_theta_;
    AABB bbox_;
};

#endif // !HITTABLE_H
