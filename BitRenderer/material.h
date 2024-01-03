/*
* 材质类
*/
#ifndef MATERIAL_H
#define MATERIAL_H

#include "ray.h"
#include "color.h"
#include "hittable.h"
#include "texture.h"
#include "onb.h"
#include "pdf.h"

struct ScatterRecord 
{
    Color attenuation;
    std::shared_ptr<PDF> pdf_ptr;
    bool skip_pdf;
    Ray skip_pdf_ray;
};

class Material 
{
public:

    virtual ~Material() = default;
    virtual bool scatter(const Ray& r_in, const HitRecord& rec, ScatterRecord& srec) const
    {
        return false;
    }
    virtual Color emitted(double u, double v, const Point3& p) const
    {
        return Color(0, 0, 0);
    }
    virtual double scattering_pdf(const Ray& r_in, const HitRecord& rec, const Ray& scattered) const 
    {
        return 0;
    }
};


class Lambertian : public Material
{
public:

    Lambertian(const Color& a) : albedo_(std::make_shared<SolidColor>(a)) {}
    Lambertian(std::shared_ptr<Texture> a) : albedo_(a) {}

    bool scatter(const Ray& r_in, const HitRecord& rec, ScatterRecord& srec) const override 
    {
        // 随机在半球上采样方向
         //Vec3 scatter_direction = random_on_hemisphere(rec.normal);
        // 朗伯分布，靠近法线的方向上反射可能性更大
        //Vec3 scatter_direction = rec.normal + random_unit_vector();

        srec.attenuation = albedo_->value(rec.u, rec.v, rec.p);
        srec.pdf_ptr = std::make_shared<CosinePDF>(rec.normal);
        srec.skip_pdf = false;
        return true;
    }

    double scattering_pdf(const Ray& r_in, const HitRecord& rec, const Ray& scattered) const
    {
        auto cosine = dot(rec.normal, unit_vector(scattered.get_direction()));
        return cosine < 0 ? 0 : cosine / kPI;
    }

private:

    std::shared_ptr<Texture> albedo_;
};


class Metal : public Material
{
public:

    Metal(const Color& a, double f) : albedo_(a), fuzz_(f < 1 ? f : 1) {}

    bool scatter(const Ray& r_in, const HitRecord& rec, ScatterRecord& srec) const override
    {
        srec.attenuation = albedo_;
        srec.pdf_ptr = nullptr;
        srec.skip_pdf = true;
        Vec3 reflected = reflect(unit_vector(r_in.get_direction()), rec.normal);
        srec.skip_pdf_ray = Ray(rec.p, reflected + fuzz_ * random_in_unit_sphere(), r_in.get_time());
        return true;
    }

private:

    Color albedo_;
    double fuzz_; // 模糊度
};

class Dielectric : public Material
{
public:

    Dielectric(double index_of_refraction) : ir_(index_of_refraction) {}

    bool scatter(const Ray& r_in, const HitRecord& rec, ScatterRecord& srec) const override
    {
        srec.attenuation = Color(1., 1., 1.);
        srec.pdf_ptr = nullptr;
        srec.skip_pdf = true;
        // 空气的折射率为1
        double refraction_ratio = rec.front_face ? (1. / ir_) : ir_;

        Vec3 unit_direction = unit_vector(r_in.get_direction());

        double cos_theta = fmin(dot(-unit_direction, rec.normal), 1.);
        double sin_theta = sqrt(1.0 - cos_theta * cos_theta);
        // 通过Snell’s law判断折射是否能发生
        bool cannot_refract = refraction_ratio * sin_theta > 1.;
        Vec3 direction;
        if (cannot_refract || reflectance(cos_theta, refraction_ratio) > random_double())
            direction = reflect(unit_direction, rec.normal);
        else
            direction = refract(unit_direction, rec.normal, refraction_ratio);

        srec.skip_pdf_ray = Ray(rec.p, direction, r_in.get_time());

        return true;
    }

private:

    double ir_; // 折射率

    // Schlick's approximation计算0度的菲涅尔折射率
    static double reflectance(double cosine, double ref_idx)
    {
        auto r0 = (1 - ref_idx) / (1 + ref_idx);
        r0 = r0 * r0;
        return r0 + (1 - r0) * pow((1 - cosine), 5);
    }
};

class DiffuseLight : public Material
{
public:

    DiffuseLight(std::shared_ptr<Texture> a) : emit_(a) {}
    DiffuseLight(Color c) : emit_(std::make_shared<SolidColor>(c)) {}

    // 自发光
    // 所有面均发光，不做背面剔除
    Color emitted(double u, double v, const Point3& p) const override
    {
        return emit_->value(u, v, p);
    }

private:

    std::shared_ptr<Texture> emit_;
};

class Isotropic : public Material
{
public:

    Isotropic(Color c) : albedo_(std::make_shared<SolidColor>(c)) {}
    Isotropic(std::shared_ptr<Texture> a) : albedo_(a) {}

    bool scatter(const Ray& r_in, const HitRecord& rec, ScatterRecord& srec) const override 
    {
        srec.attenuation = albedo_->value(rec.u, rec.v, rec.p);
        srec.pdf_ptr = std::make_shared<SpherePDF>();
        srec.skip_pdf = false;
        return true;
    }

    double scattering_pdf(const Ray& r_in, const HitRecord& rec, const Ray& scattered) const override 
    {
        return 1 / (4 * kPI);
    }

private:

    std::shared_ptr<Texture> albedo_;
};


#endif // !MATERIAL_H
