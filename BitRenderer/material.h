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
    virtual bool scatter(const Ray& r_in, const HitRecord& rec, ScatterRecord& srec) 
        const
    {
        return false;
    }

    virtual Color emitted(double u, double v, const Point3& p, const HitRecord& rec) 
        const
    {
        return Color(0, 0, 0);
    }

    virtual double scattering_pdf(const Ray& r_in, const HitRecord& rec, const Ray& scattered) 
        const 
    {
        return 0;
    }
};


class Lambertian : public Material
{
private:
    std::shared_ptr<Texture> albedo_;

public:
    Lambertian() = delete;

    ~Lambertian() = default;

    Lambertian(const Lambertian&) = delete;
    Lambertian& operator=(const Lambertian&) = delete;

    Lambertian(Lambertian&&) = delete;
    Lambertian& operator=(Lambertian&&) = delete;

    Lambertian(const Color& a) : albedo_(std::make_shared<SolidColor>(a)) {}

    Lambertian(std::shared_ptr<Texture> a) : albedo_(a) {}

public:
    bool scatter(const Ray& r_in, const HitRecord& rec, ScatterRecord& srec) 
        const override 
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

    double scattering_pdf(const Ray& r_in, const HitRecord& rec, const Ray& scattered) 
        const override
    {
        auto cosine = dot(rec.normal, unit_vector(scattered.get_direction()));
        return cosine < 0 ? 0 : cosine / kPI;
    }
};


class Metal : public Material
{
private:
    Color albedo_;
    double fuzz_; // 模糊度

public:
    Metal() = delete;

    ~Metal() = default;

    Metal(const Metal&) = delete;
    Metal& operator=(const Metal&) = delete;

    Metal(Metal&&) = delete;
    Metal& operator=(Metal&&) = delete;

    Metal(const Color& a, double f) : albedo_(a), fuzz_(f < 1 ? f : 1) {}

public:
    bool scatter(const Ray& r_in, const HitRecord& rec, ScatterRecord& srec) 
        const override
    {
        srec.attenuation = albedo_;
        srec.pdf_ptr = nullptr;
        srec.skip_pdf = true;
        Vec3 reflected = reflect(unit_vector(r_in.get_direction()), rec.normal);
        srec.skip_pdf_ray = Ray(rec.p, reflected + fuzz_ * random_in_unit_sphere(), r_in.get_time());
        return true;
    }
};

class Dielectric : public Material
{
private:
    double ir_; // 折射率

public:
    Dielectric() = delete;

    ~Dielectric() = default;

    Dielectric(const Dielectric&) = delete;
    Dielectric& operator=(const Dielectric&) = delete;

    Dielectric(Dielectric&&) = delete;
    Dielectric& operator=(Dielectric&&) = delete;

    Dielectric(double index_of_refraction) : ir_(index_of_refraction) {}

public:
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
private:
    std::shared_ptr<Texture> emit_;

public:
    DiffuseLight() = delete;

    ~DiffuseLight() = default;

    DiffuseLight(const DiffuseLight&) = delete;
    DiffuseLight& operator=(const DiffuseLight&) = delete;

    DiffuseLight(DiffuseLight&&) = delete;
    DiffuseLight& operator=(DiffuseLight&&) = delete;

    DiffuseLight(std::shared_ptr<Texture> a) : emit_(a) {}
    DiffuseLight(Color c) : emit_(std::make_shared<SolidColor>(c)) {}

public:
    // 自发光
    Color emitted(double u, double v, const Point3& p, const HitRecord& rec)
        const override
    {
        // 正面发光，背面剔除
        if (rec.front_face)
            return emit_->value(u, v, p);
        else
            return Color(0, 0, 0);
    }
};

class Isotropic : public Material
{
private:
    std::shared_ptr<Texture> albedo_;

public:
    Isotropic() = delete;

    ~Isotropic() = default;

    Isotropic(const Isotropic&) = delete;
    Isotropic& operator=(const Isotropic&) = delete;

    Isotropic(Isotropic&&) = delete;
    Isotropic& operator=(Isotropic&&) = delete;

    Isotropic(Color c) : albedo_(std::make_shared<SolidColor>(c)) {}
    Isotropic(std::shared_ptr<Texture> a) : albedo_(a) {}

public:
    bool scatter(const Ray& r_in, const HitRecord& rec, ScatterRecord& srec) 
        const override 
    {
        srec.attenuation = albedo_->value(rec.u, rec.v, rec.p);
        srec.pdf_ptr = std::make_shared<SpherePDF>();
        srec.skip_pdf = false;
        return true;
    }

    double scattering_pdf(const Ray& r_in, const HitRecord& rec, const Ray& scattered) 
        const override 
    {
        return 1 / (4 * kPI);
    }
};


#endif // !MATERIAL_H
