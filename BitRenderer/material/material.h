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
    virtual Color3 eval_color_trace(const HitRecord& rec, const Color3& next_color = Color3(), const Color3& brdf = Vec3(), const double& pdf = 1)
        = 0;

    // 光栅化计算颜色
    virtual Color3 eval_color_rasterize(const Texcoord2& uv, const Vec3& normal, const Color3& light, const Color3& ambient, const Vec3& out, const Vec3& in = Vec3())       
    {
        return { 0, 0, 0 };
    }

    // 计算新的出射光线方向
    // 若传入光源则同时对光源和材质进行重要新采样
    virtual Ray sample_ray(const Ray& r_in, const HitRecord& rec, const double& u = 0, const double& v = 0)
        const
    {
        return Ray();
    }

    // 对于已知的入射和出射光线求解BRDF
    // 对于光追，in为入射光线方向（着色点为终点），out为弹射的出射光线方向（着色点为起点）
    // 对于光栅化，in为相机方向（着色点为终点），out为光源方向（着色点为起点）
    // 本质上是对应的
    virtual Color3 eval_brdf(const Vec3& normal,  const Vec3& out, const Vec3& in = Vec3(), const double& u = 0, const double& v = 0)
    {
        return 0;
    }

    // 计算PDF值
    // 若传入光源则同时对光源和材质进行重要新采样
    virtual double eval_pdf(const Vec3& normal, const Vec3& out, const Vec3& in = Vec3(), const double& u = 0, const double& v = 0)
        const
    {
        return 0;
    }

protected:
    // 将切线空间的向量转移到世界空间
    Vec3 to_world(const Vec3& local, const Vec3& normal)
        const
    {
        Vec3 B, T;
        Vec3 N = normal;
        N.normalize();
        if (fabs(N.x()) > fabs(N.y()))
        {
            T = Vec3(N.z(), 0, -N.x());
            T = T / sqrt(N.x() * N.x() + N.z() * N.z());
        }
        else
        {
            T = Vec3(0, N.z(), -N.y());
            T = T / sqrt(N.y() * N.y() + N.z() * N.z());
        }
        T.normalize();
        B = cross(T, N);
        B.normalize();
        return local.x() * B + local.y() * T + local.z() * N;
    }
};

class Lambertian : public Material
{
private:
    shared_ptr<Texture> albedo_;

public:
    Lambertian() : albedo_(BASE_COLOR_DEFAULT) {}

    Lambertian(const Color3& a) : albedo_(make_shared<SolidColor>(a)) {}

    Lambertian(shared_ptr<Texture> a) : albedo_(a) {}

public:
    Color3 eval_color_trace(const HitRecord& rec, const Color3& next_color, const Color3& brdf, const double& pdf)
         override
    {
        return albedo_->value(rec.u, rec.v, rec.p) * next_color * brdf / pdf;
    }

    Color3 eval_color_rasterize(const Texcoord2& uv, const Vec3& normal, const Color3& light, const Color3& ambient, const Vec3& out, const Vec3& in)
        override
    {
        return albedo_->value(uv.u(), uv.v()) * light * eval_brdf(normal, out) + ambient;
    }

    Ray sample_ray(const Ray& r_in, const HitRecord& rec, const double& u, const double& v)
        const override
    {
        CosinePDF pdf(rec.normal);
        return Ray(rec.p, pdf.gen_direction(), r_in.get_time());
    }

    // Lambertian模型不需要in
    // 默认参数是静态绑定，无法在子类重写的虚函数中改变父类的虚函数指定的默认参数，不必重复写上
    // 这里重复写上默认参数是便于同类其它成员函数调用
    Color3 eval_brdf(const Vec3& normal, const Vec3& out, const Vec3& in = Vec3(), const double& u = 0, const double& v = 0)
        override
    {
        // 这里将余弦项从渲染方程移至BRDF求解函数中
        auto cosine = dot(normal, unit_vector(out));
        return cosine < 0 ? Vec3() : Vec3(cosine / kPI, cosine / kPI, cosine / kPI);
    }

    double eval_pdf(const Vec3& normal, const Vec3& out, const Vec3& in, const double& u, const double& v)
        const override
    {
        CosinePDF pdf(normal);
        return pdf.value(out);
    }
};

// 基于微表面的GGX镜面反射BRDF和Lambertian漫反射BRDF结合的材质模型
class Microfacet : public Material
{
private:
    shared_ptr<Texture> base_color_;
    shared_ptr<Texture> metallic_; // metallic、roughness应为单通道贴图，若为三通道贴图，则三通道应相同
    shared_ptr<Texture> roughness_;
    shared_ptr<Texture> normal_;

public:
    Microfacet() : 
        // 图片中像素值为[0, 255]的整数，计算中统一使用[0,1]的浮点数
        base_color_(BASE_COLOR_DEFAULT),
        metallic_(METALLIC_DEFAULT),
        roughness_(ROUGHNESS_DEFAULT),
        normal_(NORMAL_DEFAULT)
    {}

    void set_base_color(shared_ptr<Texture>&& base_color)
    {
        base_color_ = base_color;
    }

    void set_metallic(shared_ptr<Texture>&& metallic)
    {
        metallic_ = metallic;
    }

    void set_roughness(shared_ptr<Texture>&& roughness)
    {
        roughness_ = roughness;
    }

    void set_normal(shared_ptr<Texture>&& normal)
    {
        normal_ = normal;
    }

// metallic为0代表非金属，1代表纯金属
// ks应为根据F0算出来的指定角度下的F,  0.04为电介质的默认F0
// 贴图的粗糙度为BRDF使用的粗糙度alpha的平方，直接使用roughness
// 法线贴图需要将值从[0,1]映射到[-1,1]
#define KD(u, v) Color3 kd = base_color_->value(u, v) * (1 - metallic_->value(u, v)[0])
#define F0(u, v) Color3 F0 = base_color_->value(u, v) * metallic_->value(u, v)[0] + 0.04
#define ROUGHNESS(u, v) double roughness = roughness_->value(u, v)[0]
#define NORMAL_TANGENT_SPACE(u, v) Vec3 normal_tangent_space = normal_->value(u, v) * 2 - 1

public:
    Color3 eval_color_trace(const HitRecord& rec, const Color3& next_color, const Color3& brdf, const double& pdf)
        override
    {  
        double epsilon = 1e-4;
        return next_color * brdf / (pdf + epsilon);
    }

    Color3 eval_color_rasterize(const Texcoord2& uv, const Vec3& normal, const Color3& light, const Color3& ambient, const Vec3& out, const Vec3& in)
        override    
    {
        return  light * eval_brdf(normal, out, in, uv.u(), uv.v()) + ambient;
    }

    Ray sample_ray(const Ray& r_in, const HitRecord& rec, const double& u, const double& v)
        const override
    {
        ROUGHNESS(u, v);
        // GGX重要性采样
        // 算法来源: https://zhuanlan.zhihu.com/p/505284731
        float epsilon = 1e-4;
        float k_1     = random_double(), k_2 = random_double();
        // 解theta与phi
        float theta = acos(sqrt((1 - k_1) / (k_1 * (roughness * roughness - 1) + 1 + epsilon)));
        float phi   = 2 * kPI * k_2;
        // 局部坐标系
        Vec3 local_ray(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
        // 转换成世界坐标系
        Vec3 world_ray = to_world(local_ray, rec.normal);
        // 采样结果为半程向量，根据入射向量计算反射向量
        Vec3 in = r_in.get_direction();
        world_ray.normalize();
        in.normalize();
        Vec3 sample_direction = (-2 * dot(in, world_ray) * world_ray + in);
        sample_direction.normalize();
        return Ray(rec.p, sample_direction, 0);
    }

    Color3 eval_brdf(const Vec3& normal, const Vec3& out, const Vec3& in, const double& u, const double& v)
        override
    {   
        KD(u, v);
        F0(u, v);
        ROUGHNESS(u, v);
        NORMAL_TANGENT_SPACE(u, v);

        Vec3 in_n  = in;
        Vec3 out_n = out;
        Vec3 normal_n = normal;
        
        in_n.normalize();
        out_n.normalize();
        normal_n.normalize();
        // 根据法线贴图转换法线方向
        normal_n = to_world(normal_tangent_space, normal_n);
        normal_n.normalize();
        // 微表面模型: https://zhuanlan.zhihu.com/p/606074595
        // f(i,o) = F(i,h) * G(i,o,h) * D(h) / 4(n,i)(n,o)
        double cos_alpha = dot(normal_n, out_n);
        if (cos_alpha > 0)
        { 
            double epsilon = 1e-4;
            // 菲涅尔项F
            Vec3 F;
            double cos_i = dot(normal_n, -in_n);
            F = F0 + (Vec3(1, 1, 1) - F0) * pow((1 - cos_i), 5);
            // Shadowing masking G
            double alpha = roughness * roughness;
            double alpha2 = alpha * alpha;
            auto uh = [&](Vec3 w)-> double
                {
                    double theta_ = acos(dot(w, normal_n));
                    return (-1 + sqrt(1 + alpha2 * tan(theta_) * tan(theta_))) / 2;
                };
            double G = 1 / (1 + uh(-in_n) + uh(out_n) + epsilon);
            // 法线分布 D
            Vec3 h = -in_n + out_n;
            h.normalize();
            double cos_theta = dot(h, normal_n);
            double D = alpha2 / kPI / (pow(((alpha2 - 1) * cos_theta * cos_theta + 1), 2) + epsilon);
            
            // 这里将余弦项从渲染方程移至BRDF求解函数中，消掉分母中的dot(normal_n, out_n)
            Vec3 specular = F * G * D / (4 * dot(normal_n, -in_n) + epsilon);
            Vec3 diffuse = (Vec3(1, 1, 1) - F) * kd / kPI * dot(normal_n, out_n);
            return (diffuse + specular);
        }
        else
        {
            return Vec3(0, 0, 0);
        }
    }

    double eval_pdf(const Vec3& normal, const Vec3& out, const Vec3& in, const double& u, const double& v)
        const override
    {
        ROUGHNESS(u, v);
        // 需要将h的pdf转换为out的pdf
        if (dot(out, normal) > 0)
        {
            Vec3 in_n  = in;
            Vec3 out_n = out;
            Vec3 normal_n = normal;
            in_n.normalize();
            out_n.normalize();
            normal_n.normalize();

            Vec3 h = -in_n + out_n;
            h.normalize();
            double epsilon   = 1e-4;
            double cos_theta = dot(h, normal_n);
            double d         = roughness * roughness / kPI / (pow(((roughness * roughness - 1) * cos_theta * cos_theta + 1), 2) + epsilon);
            double pdf       = d * cos_theta / (4 * dot(-in_n, h)  + epsilon);
            return pdf;
        }
        else
        {
            return 0;
        }
    }
#undef KD
#undef F0
#undef ROUGHNESS
#undef NORMAL_TANGENT_SPACE
};

class Isotropic : public Material
{
private:
    shared_ptr<Texture> albedo_;

public:
    Isotropic(Color3 c) : albedo_(make_shared<SolidColor>(c)) {}
    Isotropic(shared_ptr<Texture> a) : albedo_(a) {}

public:
    Color3 eval_color_trace(const HitRecord& rec, const Color3& next_color, const Color3& brdf, const double& pdf)
        override
    {
        return albedo_->value(rec.u, rec.v, rec.p) * next_color * brdf / pdf;
    }

    Ray sample_ray(const Ray& r_in, const HitRecord& rec, const double& u, const double& v)
        const override
    {
        SpherePDF pdf;
        return Ray(rec.p, pdf.gen_direction(), r_in.get_time());
    }

    Color3 eval_brdf(const Vec3& normal, const Vec3& out, const Vec3& in, const double& u, const double& v)
        override
    {
        return Vec3(1 / (4 * kPI), 1 / (4 * kPI), 1 / (4 * kPI));
    }

    double eval_pdf(const Vec3& normal, const Vec3& out, const Vec3& in, const double& u, const double& v)
        const override
    {
        SpherePDF pdf;
        return pdf.value(out);
    }
};

class Metal : public Material
{
private:
    Color3 albedo_;
    double fuzz_;

public:
    Metal(const Color3& a, double f) : albedo_(a), fuzz_(f < 1 ? f : 1) 
    {
        skip_pdf_ = true;
    }

public:
    Color3 eval_color_trace(const HitRecord& rec, const Color3& next_color, const Color3& brdf, const double& pdf)
        override
    {
        return albedo_ * next_color;
    }

    Ray sample_ray(const Ray& r_in, const HitRecord& rec, const double& u, const double& v)
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
    Color3 eval_color_trace(const HitRecord& rec, const Color3& next_color, const Color3& brdf, const double& pdf)
        override
    {
        return Color3({ 1., 1., 1. }) * next_color;
    }

    Ray sample_ray(const Ray& r_in, const HitRecord& rec, const double& u, const double& v)
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
    Color3 eval_color_trace(const HitRecord& rec, const Color3& next_color, const Color3& brdf, const double& pdf)
        override
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
