/*
* 材质类
*/
#ifndef MATERIAL_H
#define MATERIAL_H

#include "ray.h"
#include "color.h"
#include "hittable.h"

class Material 
{
public:

    virtual ~Material() = default;
    virtual bool scatter(const Ray& r_in, const HitRecord& rec, Color& attenuation, Ray& scattered) const = 0;
};


class Lambertian : public Material
{
public:

    Lambertian(const Color& a) : albedo_(a) {}

    bool scatter(const Ray& r_in, const HitRecord& rec, Color& attenuation, Ray& scattered) const override 
    {
        // 随机在半球上采样方向
        // Vec3 scatter_direction = random_on_hemisphere(rec.normal);
        // 朗伯分布，靠近法线的方向上反射可能性更大
        Vec3 scatter_direction = rec.normal + random_unit_vector();

        // 排除边界情况
        if (scatter_direction.near_zero())
            scatter_direction = rec.normal;

        scattered = Ray(rec.p, scatter_direction);
        attenuation = albedo_;
        return true;
    }

private:

    Color albedo_;
};


class Metal : public Material
{
public:

    Metal(const Color& a, double f) : albedo_(a), fuzz_(f < 1 ? f : 1) {}

    bool scatter(const Ray& r_in, const HitRecord& rec, Color& attenuation, Ray& scattered) const override
    {
        Vec3 reflected = reflect(unit_vector(r_in.get_direction()), rec.normal);
        // 添加fuzz_效果
        scattered = Ray(rec.p, reflected + fuzz_ * random_unit_vector());
        attenuation = albedo_;
        return (dot(scattered.get_direction(), rec.normal) > 0);
    }

private:

    Color albedo_;
    double fuzz_;
};

#endif // !MATERIAL_H
