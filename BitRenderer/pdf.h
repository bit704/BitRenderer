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
    virtual ~PDF() = default;
    virtual double value(const Vec3& direction) const = 0;
    virtual Vec3 generate() const = 0;
};

class SpherePDF : public PDF
{
public:
    SpherePDF() = default;

    ~SpherePDF() = default;

    SpherePDF(const SpherePDF&) = default;
    SpherePDF& operator=(const SpherePDF&) = default;

    SpherePDF(SpherePDF&&) = default;
    SpherePDF& operator=(SpherePDF&&) = default;

public:
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
private:
    ONB uvw_;

public:
    CosinePDF() = default;

    ~CosinePDF() = default;

    CosinePDF(const CosinePDF&) = default;
    CosinePDF& operator=(const CosinePDF&) = default;

    CosinePDF(CosinePDF&&) = default;
    CosinePDF& operator=(CosinePDF&&) = default;

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
};

// 用于光源采样
class HittablePDF : public PDF 
{
private:
    const Hittable& objects_;
    Point3 origin_;

public:
    HittablePDF() = delete;

    ~HittablePDF() = default;

    HittablePDF(const HittablePDF&) = delete;
    HittablePDF& operator=(const HittablePDF&) = delete;

    HittablePDF(HittablePDF&&) = delete;
    HittablePDF& operator=(HittablePDF&&) = delete;

    HittablePDF(const Hittable& _objects, const Point3& _origin)
        : objects_(_objects), origin_(_origin) {}

public:
    double value(const Vec3& direction) 
        const override
    {
        return objects_.pdf_value(origin_, direction);
    }

    Vec3 generate() 
        const override
    {
        return objects_.random(origin_);
    }
};

// 加权混合PDF
class MixturePDF : public PDF
{
private:
    std::shared_ptr<PDF> p_[2];

public:
    MixturePDF() = delete;

    ~MixturePDF() = default;

    MixturePDF(const MixturePDF&) = delete;
    MixturePDF& operator=(const MixturePDF&) = delete;

    MixturePDF(MixturePDF&&) = delete;
    MixturePDF& operator=(MixturePDF&&) = delete;

    MixturePDF(std::shared_ptr<PDF> p0, std::shared_ptr<PDF> p1)
    {
        p_[0] = p0;
        p_[1] = p1;
    }

public:
    double value(const Vec3& direction) 
        const override
    {
        return 0.5 * p_[0]->value(direction) + 0.5 * p_[1]->value(direction);
    }

    Vec3 generate() 
        const override 
    {
        if (random_double() < 0.5)
            return p_[0]->generate();
        else
            return p_[1]->generate();
    }
};

#endif // !PDF_H