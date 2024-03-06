/*
 * 材质类
 */
#ifndef MATERIAL_H
#define MATERIAL_H

#include "vec.h"
#include "hittable.h"
#include "onb.h"
#include "pdf.h"
#include "ray.h"
#include "texture.h"

class Material 
{
public:
    bool skip_pdf_   = false; // 无需重要性采样，直接反射/折射
    bool no_scatter_ = false; // 光线击中后不发生散射，如光源

    // 光追计算颜色
    // 因为计算颜色时不止对材质进行重要性采样，还会对光源等其它物体进行重要新采样，brdf和pdf值会改变，因此作为参数输入而不是直接内部计算
    virtual Color3 eval_color_trace(const HitRecord& rec, const Color3& next_color = Color3(), double brdf = 0., double pdf = 1.)
        const = 0;

    // 光栅化计算颜色
    virtual Color3 eval_color_rasterize(const Texcoord2& uv, const Vec3& normal, const Color3& light, const Color3& ambient, const Vec3& out, const Vec3& in = Vec3())
        const
    {
        return { 0, 0, 0 };
    }

    // 计算新的出射光线方向
    // 若传入光源则同时对光源和材质进行重要新采样
    virtual Ray sample_ray(const Ray& r_in, const HitRecord& rec)
        const
    {
        return Ray();
    }

    // 对于已知的入射和出射光线求解BRDF
    // 对于光追，in为入射光线方向，out为弹射的出射光线方向（方向起点均为击中点）
    // 对于光栅化，in为着色点到相机方向，out为着色点到光源方向
    // 本质上是对应的
    virtual double eval_brdf(const Vec3& normal, const Vec3& out, const Vec3& in = Vec3())
        const
    {
        return 0;
    }

    // 计算PDF值
    // 若传入光源则同时对光源和材质进行重要新采样
    virtual double eval_pdf(const Vec3& normal, const Vec3& out)
        const
    {
        return 0;
    }
};

class Lambertian : public Material
{
private:
    shared_ptr<Texture> albedo_;

public:
    Lambertian(const Color3& a) : albedo_(std::make_shared<SolidColor>(a)) {}

    Lambertian(shared_ptr<Texture> a) : albedo_(a) {}

    Lambertian(const Lambertian& other) : albedo_(other.albedo_) {}
    Lambertian& operator=(const Lambertian& other)
    {
        albedo_ = other.albedo_;
    }

    Lambertian(Lambertian&& other) noexcept : albedo_(std::move(other.albedo_)) {};
    Lambertian& operator=(Lambertian&& other) noexcept
    {
        albedo_ = std::move(other.albedo_);
    }

public:
    Color3 eval_color_trace(const HitRecord& rec, const Color3& next_color, double brdf, double pdf)
         const override
    {
        return albedo_->value(rec.u, rec.v, rec.p) * next_color * brdf / pdf;
    }

    Color3 eval_color_rasterize(const Texcoord2& uv, const Vec3& normal, const Color3& light, const Color3& ambient, const Vec3& out, const Vec3& in)
        const override
    {
        return albedo_->value(uv.u(), uv.v()) * light * eval_brdf(normal, out) + ambient;
    }

    Ray sample_ray(const Ray& r_in, const HitRecord& rec)
        const override
    {
        CosinePDF pdf(rec.normal);
        return Ray(rec.p, pdf.gen_direction(), r_in.get_time());
    }

    // Lambertian模型不需要in
    // 默认参数是静态绑定，无法在子类重写的虚函数中改变父类的虚函数指定的默认参数，不必重复写上
    // 这里加上默认参数是便于其它同类成员函数调用
    double eval_brdf(const Vec3& normal, const Vec3& out, const Vec3& in = Vec3())
        const override
    {
        auto cosine = dot(normal, unit_vector(out));
        return cosine < 0 ? 0 : cosine / kPI;
    }

    double eval_pdf(const Vec3& normal, const Vec3& out)
        const override
    {
        CosinePDF pdf(normal);
        return pdf.value(out);
    }
};

class Isotropic : public Material
{
private:
    shared_ptr<Texture> albedo_;

public:
    Isotropic(Color3 c) : albedo_(std::make_shared<SolidColor>(c)) {}
    Isotropic(shared_ptr<Texture> a) : albedo_(a) {}

    Isotropic(const Isotropic& other) : albedo_(other.albedo_) {}
    Isotropic& operator=(const Isotropic& other)
    {
        albedo_ = other.albedo_;
    }

    Isotropic(Isotropic&& other) noexcept : albedo_(std::move(other.albedo_)) {}
    Isotropic& operator=(Isotropic&& other) noexcept
    {
        albedo_ = std::move(other.albedo_);
    }

public:
    Color3 eval_color_trace(const HitRecord& rec, const Color3& next_color, double brdf, double pdf)
        const override
    {
        return albedo_->value(rec.u, rec.v, rec.p) * next_color * brdf / pdf;
    }

    Ray sample_ray(const Ray& r_in, const HitRecord& rec)
        const override
    {
        SpherePDF pdf;
        return Ray(rec.p, pdf.gen_direction(), r_in.get_time());
    }

    double eval_brdf(const Vec3& normal, const Vec3& out, const Vec3& in)
        const override
    {
        return 1 / (4 * kPI);
    }

    double eval_pdf(const Vec3& normal, const Vec3& out)
        const override
    {
        SpherePDF pdf;
        return pdf.value(out);
    }
};

class Metal : public Material
{
private:
    Color3  albedo_;
    double fuzz_;

public:
    Metal(const Color3& a, double f) : albedo_(a), fuzz_(f < 1 ? f : 1) 
    {
        skip_pdf_ = true;
    }

public:
    Color3 eval_color_trace(const HitRecord& rec, const Color3& next_color, double brdf, double pdf)
        const override
    {
        return albedo_ * next_color;
    }

    Ray sample_ray(const Ray& r_in, const HitRecord& rec)
        const override
    {
        Vec3 reflected = reflect(unit_vector(r_in.get_direction()), rec.normal);
        return Ray(rec.p, reflected + fuzz_ * random_in_unit_sphere(), r_in.get_time());
    }
};

class Dielectric : public Material
{
private:
    double ir_; // 折射率

public:
    Dielectric(double index_of_refraction) : ir_(index_of_refraction)
    {
        skip_pdf_ = true;
    }

public:
    Color3 eval_color_trace(const HitRecord& rec, const Color3& next_color, double brdf, double pdf)
        const override
    {
        return Color3({ 1., 1., 1. }) * next_color;
    }

    Ray sample_ray(const Ray& r_in, const HitRecord& rec)
        const override
    {
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

        return Ray(rec.p, direction, r_in.get_time());
    }

private:
    // Schlick's approximation计算0度的菲涅尔反射率
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
    shared_ptr<Texture> emit_;

public:
    DiffuseLight(shared_ptr<Texture> a) : emit_(a)
    {
        no_scatter_ = true;
    }
    DiffuseLight(Color3 c) : emit_(std::make_shared<SolidColor>(c))
    {
        no_scatter_ = true;
    }

    DiffuseLight(const DiffuseLight& other) : emit_(other.emit_) {}
    DiffuseLight& operator=(const DiffuseLight& other)
    {
        emit_ = other.emit_;
    }

    DiffuseLight(DiffuseLight&& other) noexcept : emit_(std::move(other.emit_)) {}
    DiffuseLight& operator=(DiffuseLight&& other) noexcept
    {
        emit_ = std::move(other.emit_);
    }

public:
    Color3 eval_color_trace(const HitRecord& rec, const Color3& next_color, double brdf, double pdf)
        const override
    {
        Color3 this_color;
        // 正面发光，背面剔除
        if (rec.front_face)
            this_color =  emit_->value(rec.u, rec.v, rec.p);
        else
            this_color =  Color3(0, 0, 0);

        return this_color;
    }
};
#endif // !MATERIAL_H
