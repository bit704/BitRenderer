/*
 * 概率密度函数类
 * Probability Density Function
 */
#ifndef PDF_H
#define PDF_H

#include "hittable_list.h"
#include "onb.h"

class PDF
{
public:
    virtual ~PDF() = default;
    virtual double value(const Vec3& direction) const = 0;
    virtual Vec3 gen_direction() const = 0;
};

class SpherePDF : public PDF
{
public:
    double value(const Vec3& direction) const override 
    {
        return 1 / (4 * kPI);
    }

    Vec3 gen_direction() const override
    {
        return random_unit_vector();
    }
};

class CosinePDF : public PDF
{
private:
    ONB uvw_;

public:
    CosinePDF(const Vec3 & w) 
    {
        uvw_.build_from_w(w); 
    }

    double value(const Vec3 & direction) const override
    {
        auto cosine_theta = dot(unit_vector(direction), uvw_.w());
        return fmax(0, cosine_theta / kPI);
    }

    Vec3 gen_direction() const override
    {
        return uvw_.local(random_cosine_direction());
    }
};

// 用于对可击中物体表面采样
class HittablePDF : public PDF 
{
private:
    const Hittable& objects_;
    Point3 origin_;

public:
    HittablePDF(const Hittable& _objects, const Point3& _origin)
        : objects_(_objects), origin_(_origin) {}

public:
    double value(const Vec3& direction) 
        const override
    {
        return objects_.pdf_value(origin_, direction);
    }

    Vec3 gen_direction() 
        const override
    {
        return objects_.random(origin_);
    }
};

#endif // !PDF_H