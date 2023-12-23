/*
* 抽象可击中物体
*/
#ifndef HITTABLE_H
#define HITTABLE_H

#include "ray.h"

struct HitRecord
{
    Point3 p;
    Vec3 normal;
    double t;
    bool front_face;

    // 确保法线面向观察者
    // outward_normal为从球心指向光线击中点方向的法线
    void set_face_normal(const Ray& r, const Vec3& outward_normal) 
    {
        front_face = dot(r.get_direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

// 纯虚类
class HitTable
{
public:

    virtual ~HitTable() = default;
    virtual bool hit(const Ray& r, double ray_tmin, double ray_tmax, HitRecord& rec) const = 0;
};

#endif // !HITTABLE_H
