/*
* 光线可击中物体总列表类
*/
#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include <memory>
#include <vector>

#include "hittable.h"

class HittableList : public Hittable
{
public:

    HittableList() = default;

    HittableList(std::shared_ptr<Hittable> object) 
    { 
        add(object);
    }

    void clear() 
    { 
        objects_.clear(); 
    }

    void add(std::shared_ptr<Hittable> object)
    {
        objects_.push_back(object);
        bbox_ = AABB(bbox_, object->get_bbox()); // 并集运算
    }

    bool hit(const Ray& r, Interval interval, HitRecord& rec) const override
    {
        HitRecord temp_rec;
        bool hit_anything = false;
        auto closest_so_far = interval.get_max();

        for (const auto& object : objects_)
        {
            if (object->hit(r, Interval(interval.get_min(), closest_so_far), temp_rec))
            {
                hit_anything = true;
                closest_so_far = temp_rec.t; // 更新当前击中时候的光线投射距离为下次光线投射的最大距离
                rec = temp_rec; // 更新击中信息
            }
        }

        return hit_anything;
    }

    std::vector<std::shared_ptr<Hittable>> get_objects() const
    {
        return objects_;
    }

    // 多个物体PDF加权
    double pdf_value(const Point3& o, const Vec3& v) const override
    {
        auto weight = 1. / objects_.size();
        auto sum = 0.;

        for (const auto& object : objects_)
            sum += weight * object->pdf_value(o, v);

        return sum;
    }

    Vec3 random(const Vec3& o) const override
    {
        auto int_size = static_cast<int>(objects_.size());
        return objects_[random_int(0, int_size - 1)]->random(o);
    }

    AABB get_bbox() const override
    {
        return bbox_;
    }

private:

    std::vector<std::shared_ptr<Hittable>> objects_;
    AABB bbox_;
};

#endif // !HITTABLE_LIST_H
