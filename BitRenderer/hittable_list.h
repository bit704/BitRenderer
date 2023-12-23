/*
* 与光线相交物体总列表
*/
#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include "hittable.h"

#include <memory>
#include <vector>

using std::shared_ptr;
using std::make_shared;

class HittableList : public HitTable
{
public:

    HittableList() = default;
    HittableList(shared_ptr<HitTable> object) 
    { 
        add(object);
    }

    void clear() 
    { 
        objects_.clear(); 
    }

    void add(shared_ptr<HitTable> object) 
    {
        objects_.push_back(object);
    }

    bool hit(const Ray& r, double ray_tmin, double ray_tmax, HitRecord& rec) const override
    {
        HitRecord temp_rec;
        bool hit_anything = false;
        auto closest_so_far = ray_tmax;

        for (const auto& object : objects_)
        {
            if (object->hit(r, ray_tmin, closest_so_far, temp_rec)) 
            {
                hit_anything = true;
                closest_so_far = temp_rec.t; // 更新当前击中时候的光线投射距离为下次光线投射的最大距离
                rec = temp_rec; // 更新击中信息
            }
        }

        return hit_anything;
    }

private:

    std::vector<shared_ptr<HitTable>> objects_;
};

#endif // !HITTABLE_LIST_H
