/*
* 概率密度函数类
* Probability Density Function
*/
#ifndef PDF_H
#define PDF_H

#include "onb.h"
#include "hittable_list.h"

class PDF
{
public:

    virtual ~PDF() {}
    virtual double value(const Vec3& direction) const = 0;
    virtual Vec3 generate() const = 0;
};

class SpherePDF : public PDF
{
public:

    SpherePDF() { }

    double value(const Vec3& direction) const override 
    {
        return 1 / (4 * kPI);
    }

    Vec3 generate() const override
    {
        return random_unit_vector();
    }
};

class CosinePDF : public PDF
{
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

    Vec3 generate() const override
    {
        return uvw_.local(random_cosine_direction());
    }

private:
      
    ONB uvw_;
};

// 用于光源采样
class HittablePDF : public PDF 
{
public:

    HittablePDF(const Hittable& _objects, const Point3& _origin)
        : objects_(_objects), origin_(_origin) {}

    double value(const Vec3& direction) const override
    {
        return objects_.pdf_value(origin_, direction);
    }

    Vec3 generate() const override
    {
        return objects_.random(origin_);
    }

private:

    const Hittable& objects_;
    Point3 origin_;
};

// 加权混合PDF
class MixturePDF : public PDF
{
public:

    MixturePDF(std::shared_ptr<PDF> p0, std::shared_ptr<PDF> p1)
    {
        p_[0] = p0;
        p_[1] = p1;
    }

    double value(const Vec3& direction) const override
    {
        return 0.5 * p_[0]->value(direction) + 0.5 * p_[1]->value(direction);
    }

    Vec3 generate() const override 
    {
        if (random_double() < 0.5)
            return p_[0]->generate();
        else
            return p_[1]->generate();
    }

private:

    std::shared_ptr<PDF> p_[2];
};

#endif // !PDF_H