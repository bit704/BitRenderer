/*
 * 恒定介质
 */
#ifndef CONSTANT_MEDIUM_H
#define CONSTANT_MEDIUM_H

#include "common.h"
#include "logger.h"
#include "material.h"

class ConstantMedium : public Hittable
{
private:
    shared_ptr<Hittable> boundary_;
    shared_ptr<Material> phase_function_;
    double neg_inv_density_;

public:
    ConstantMedium(shared_ptr<Hittable> b, double d, shared_ptr<Texture> a)
        : boundary_(b), neg_inv_density_(-1 / d), phase_function_(std::make_shared<Isotropic>(a)) {}

    ConstantMedium(shared_ptr<Hittable> b, double d, Color3 c)
        : boundary_(b), neg_inv_density_(-1 / d), phase_function_(std::make_shared<Isotropic>(c)) {}

    ConstantMedium(const ConstantMedium&) = delete;
    ConstantMedium& operator=(const ConstantMedium&) = delete;

    ConstantMedium(ConstantMedium&&) = delete;
    ConstantMedium& operator=(ConstantMedium&&) = delete;

public: 
    bool hit(const Ray& r, const Interval& interval, HitRecord& rec)
        const override
    {
        // debug
        const bool enableDebug = false;
        const bool debugging = enableDebug && random_double() < 1e-5;

        // 求与碰撞体的前后两个交点
        HitRecord rec1, rec2;

        if (!boundary_->hit(r, Interval(-kInfinitDouble, kInfinitDouble), rec1))
            return false;

        if (!boundary_->hit(r, Interval(rec1.t + 1e-4, kInfinitDouble), rec2))
            return false;

        if (debugging)
        {
            LOG("ray_tmin=", rec1.t, ", ray_tmax=", rec2.t);
        }
            

        if (rec1.t < interval.get_min()) 
            rec1.t = interval.get_min();
        if (rec2.t > interval.get_max()) 
            rec2.t = interval.get_max();

        if (rec1.t >= rec2.t)
            return false;

        if (rec1.t < 0)
            rec1.t = 0;

        auto ray_step_length = r.get_direction().norm(); // 步长
        auto distance_inside_boundary = (rec2.t - rec1.t) * ray_step_length;
        auto hit_distance = neg_inv_density_ * log(random_double());

        if (hit_distance > distance_inside_boundary)
            return false;

        rec.t = rec1.t + hit_distance / ray_step_length;
        rec.p = r.at(rec.t);

        if (debugging)
        {
            LOG("hit_distance = ", hit_distance);
            LOG("rec.t = ", rec.t);
            LOG("rec.p = ", rec.p);
        }

        rec.normal = Vec3(1, 0, 0);  // 任意
        rec.front_face = true;     // 任意
        rec.material = phase_function_;

        return true;
    }

    AABB get_bbox() 
        const override
    {
        return boundary_->get_bbox(); 
    }
};

#endif // !CONSTANT_MEDIUM_H
