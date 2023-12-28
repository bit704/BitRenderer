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

        scattered = Ray(rec.p, scatter_direction, r_in.get_time());
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
        scattered = Ray(rec.p, reflected + fuzz_ * random_unit_vector(), r_in.get_time());
        attenuation = albedo_;
        return (dot(scattered.get_direction(), rec.normal) > 0);
    }

private:

    Color albedo_;
    double fuzz_; // 模糊度
};

class Dielectric : public Material
{
public:

    Dielectric(double index_of_refraction) : ir(index_of_refraction) {}

    bool scatter(const Ray& r_in, const HitRecord& rec, Color& attenuation, Ray& scattered) const override
    {
        attenuation = Color(1.0, 1.0, 1.0);
        // 空气的折射率为1
        double refraction_ratio = rec.front_face ? (1.0 / ir) : ir;

        Vec3 unit_direction = unit_vector(r_in.get_direction());

        double cos_theta = fmin(dot(-unit_direction, rec.normal), 1.0);
        double sin_theta = sqrt(1.0 - cos_theta * cos_theta);
        // 通过Snell’s law判断折射是否能发生
        bool cannot_refract = refraction_ratio * sin_theta > 1.0;
        Vec3 direction;
        if (cannot_refract || reflectance(cos_theta, refraction_ratio) > random_double())
            direction = reflect(unit_direction, rec.normal);
        else
            direction = refract(unit_direction, rec.normal, refraction_ratio);

        scattered = Ray(rec.p, direction, r_in.get_time());

        return true;
    }

private:

    double ir; // 折射率

    // Schlick's approximation计算0度的菲涅尔折射率
    static double reflectance(double cosine, double ref_idx)
    {
        auto r0 = (1 - ref_idx) / (1 + ref_idx);
        r0 = r0 * r0;
        return r0 + (1 - r0) * pow((1 - cosine), 5);
    }

};

#endif // !MATERIAL_H
