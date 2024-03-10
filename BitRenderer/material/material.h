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
    virtual Color3 eval_color_trace(const HitRecord& rec, const Color3& next_color = Color3(), Color3 brdf = Vec3(0, 0, 0), double pdf = 1., const Ray in = Ray())
        = 0;

    // 光栅化计算颜色
    virtual Color3 eval_color_rasterize(const Texcoord2& uv, const Vec3& normal, const Color3& light, const Color3& ambient, const Vec3& out, const Vec3& in = Vec3())       
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
    virtual Color3 eval_brdf(const Vec3& normal,  const Vec3& out, const Vec3& in = Vec3(), const double u = 0, const double v = 0 )
    {
        return 0;
    }

    // 计算PDF值
    // 若传入光源则同时对光源和材质进行重要新采样
    virtual double eval_pdf(const Vec3& normal, const Vec3& out, const Vec3& in)
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
    Lambertian() : albedo_(std::make_shared<SolidColor>(Color3(0, 1, 0))) {}

    Lambertian(const Color3& a) : albedo_(std::make_shared<SolidColor>(a)) {}

    Lambertian(shared_ptr<Texture> a) : albedo_(a) {}

public:
    Color3 eval_color_trace(const HitRecord& rec, const Color3& next_color, Color3 brdf, double pdf, const Ray in)
         override
    {
        return albedo_->value(rec.u, rec.v, rec.p) * next_color * brdf / pdf;
    }

    Color3 eval_color_rasterize(const Texcoord2& uv, const Vec3& normal, const Color3& light, const Color3& ambient, const Vec3& out, const Vec3& in)
        override
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
    // 这里重复写上默认参数是便于同类其它成员函数调用
    Color3 eval_brdf(const Vec3& normal, const Vec3& out, const Vec3& in = Vec3(), const double u = 0, const double v = 0)
        override
    {
        auto cosine = dot(normal, unit_vector(out));
        return cosine < 0 ? Vec3(0, 0, 0) : Vec3(cosine / kPI, cosine / kPI, cosine / kPI);
    }

    double eval_pdf(const Vec3& normal, const Vec3& out, const Vec3& in)
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
    shared_ptr<Texture> metallic_;
    shared_ptr<Texture> roughness_;
    shared_ptr<Texture> normal_;
    // 内部计算用参数
    Color3 kd = Color3();
    Color3 F0 = Color3(); 
    double roughness = 0;

public:
    Microfacet() : 
        // 图片中像素值为[0, 255]的整数，计算中统一使用[0,1]的浮点数
        base_color_(make_shared<SolidColor>(Color3(0, 1, 0))),
        metallic_(make_shared<SolidColor>(Color3(0, 0, 0))),
        roughness_(make_shared<SolidColor>(Color3(.5, .5, .5))),
        normal_(make_shared<SolidColor>(Color3(0, 0, 1)))
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

private:
    void parameter_mapping(const double& u, const double& v)
    {
        static constexpr double dielectric_default_f0 = 0.04;

        Color3 base_color_value = base_color_->value(u, v);
        double metallic_value = metallic_->value(u, v)[0]; // metallic、roughness应为单通道贴图，若为三通道贴图，则三通道应相同
        double roughness_value = roughness_->value(u, v)[0];
        Vec3 normal_value = normal_->value(u, v);

        kd = base_color_->value(u, v) * (1 - metallic_value);  // metallic为0代表非金属，1代表纯金属
        F0 = base_color_->value(u, v) * metallic_value + dielectric_default_f0; // ks应为根据F0算出来的指定角度下的F
        roughness = roughness_value; // 贴图的粗糙度为BRDF使用的粗糙度alpha的平方 改为直接使用roughness       
    }

public:
    Color3 eval_color_trace(const HitRecord& rec, const Color3& next_color, Color3 brdf, double pdf, const Ray in)
        override
    {  
        parameter_mapping(rec.u, rec.v);
        float epsilon = 0.0001;
        Vec3 in_dir = in.get_direction();
        Vec3 n = rec.normal;
        in_dir.normalize();
        n.normalize();
        return next_color * brdf * dot(in_dir, n) / (pdf + epsilon);
    }

    Color3 eval_color_rasterize(const Texcoord2& uv, const Vec3& normal, const Color3& light, const Color3& ambient, const Vec3& out, const Vec3& in)
        override    
    {
        parameter_mapping(uv.u(), uv.v());
        return  light * eval_brdf(normal, out, in, uv.u(), uv.v()) + ambient;
    }

    Ray sample_ray(const Ray& r_in, const HitRecord& rec)
        const override
    {
        // GGX重要性采样
        // 算法来源: https://zhuanlan.zhihu.com/p/505284731
        float epsilon = 0.0001;
        float k_1     = random_double(), k_2 = random_double();
        // 解theta与phi
        float theta = acos(std::sqrt((1 - k_1) / (k_1 * (roughness - 1) + 1 + epsilon)));
        float phi   = 2 * kPI * k_2;
        // 局部坐标系
        Vec3 localRay(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
        // 转换成世界坐标系
        Vec3 B, T;
        Vec3 N = rec.normal;
        N.normalize();
        if (std::fabs( N.x()) > std::fabs( N.y()))
        {
            T = Vec3(N.z(), 0.0f, -N.x());
            T =  T / std::sqrt(N.x() * N.x() + N.z() * N.z());
        }
        else 
        {
            T = Vec3(0.0f, N.z(), -N.y());
            T = T / std::sqrt(N.y() * N.y() + N.z() * N.z());
        }
        T.normalize();
        B = cross(T, N);
        B.normalize();
        Vec3 worldRay = localRay.x() * B + localRay.y() * T + localRay.z() * N;
        // 采样结果为半程向量，根据入射向量计算反射向量
        Vec3 in = r_in.get_direction();
        worldRay.normalize();
        in.normalize();
        Vec3 sample_direction = (-2 * dot(in, worldRay) * worldRay + in);
        sample_direction.normalize();
        return Ray(rec.p, sample_direction, 0);
    }

    Color3 eval_brdf(const Vec3& normal, const Vec3& out, const Vec3& in, const double u, const double v)
        override
    {   
        parameter_mapping(u, v);
        
        Vec3 in_  = in;
        Vec3 out_ = out;
        Vec3 n = normal;
        
        in_.normalize();
        out_.normalize();
        n.normalize();

        // 微表面模型: https://zhuanlan.zhihu.com/p/606074595
        // f(i,o) = F(i,h) * G(i,o,h) * D(h) / 4(n,i)(n,o)
        float cosalpha = dot(n, out_);
        if (cosalpha > 0.0f)
        { 
            float epsilon = 0.0001;
            // 菲涅尔项F
            Vec3 F;
            float cos_i = dot(n, -in_);
            F = F0 + (Vec3(1., 1., 1.) - F0) * pow((1 - cos_i), 5);
            //std::cout << F << std::endl;
            // Shadowing masking G
            auto uh = [&](Vec3 w)-> float
                {
                    float theta_ = acos(dot(w, n));
                    return (-1 + std::sqrt(1 + roughness * tan(theta_) * tan(theta_))) / 2;
                };
            float G = 1 / (1 + uh(-in_) + uh(out_) + epsilon);
            // 法线分布 D
            Vec3 h = -in_ + out_;
            h.normalize();
            float cos_theta = dot(h, n);
            float D = roughness / kPI / (pow(((roughness - 1) * cos_theta * cos_theta + 1), 2) + epsilon);
            
            Vec3 specular = F * G * D / (4 * dot(n, -in_) * dot(n, out_) + epsilon);
            Vec3 diffuse = (Vec3(1.0f, 1.0f, 1.0f) - F) * kd / kPI;
            return (diffuse + specular);
        }
        else
            return Vec3(0, 0, 0);
    }

    double eval_pdf(const Vec3& normal, const Vec3& out, const Vec3& in )
        const override
    {
        // 需要将h的pdf转换为out的pdf
        if (dot(out, normal) > 0.0f)
        {
            Vec3 in_  = in;
            Vec3 out_ = out;
            Vec3 n = normal;
            in_.normalize();
            out_.normalize();
            n.normalize();

            Vec3 h = -in_ + out_;
            h.normalize();
            float  epsilon   = 0.0001;
            float  cos_theta = dot(h, n);
            float  d         = roughness / kPI / (pow(((roughness - 1) * cos_theta * cos_theta + 1), 2) + epsilon);
            double pdf       = d * cos_theta / (4 * dot(-in_, h)  + epsilon);
            return pdf;
        }
        else
            return 0.0f;
    }
};

class Isotropic : public Material
{
private:
    shared_ptr<Texture> albedo_;

public:
    Isotropic(Color3 c) : albedo_(make_shared<SolidColor>(c)) {}
    Isotropic(shared_ptr<Texture> a) : albedo_(a) {}

public:
    Color3 eval_color_trace(const HitRecord& rec, const Color3& next_color, Color3 brdf, double pdf, const Ray in)
        override
    {
        return albedo_->value(rec.u, rec.v, rec.p) * next_color * brdf / pdf;
    }

    Ray sample_ray(const Ray& r_in, const HitRecord& rec)
        const override
    {
        SpherePDF pdf;
        return Ray(rec.p, pdf.gen_direction(), r_in.get_time());
    }

    Color3 eval_brdf(const Vec3& normal, const Vec3& out, const Vec3& in, const double u = 0, const double v = 0)
        override
    {
        return Vec3(1 / (4 * kPI), 1 / (4 * kPI), 1 / (4 * kPI));
    }

    double eval_pdf(const Vec3& normal, const Vec3& out, const Vec3& in)
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
    Color3 eval_color_trace(const HitRecord& rec, const Color3& next_color, Color3 brdf, double pdf, const Ray in)
        override
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
    Color3 eval_color_trace(const HitRecord& rec, const Color3& next_color, Color3 brdf, double pdf, const Ray in)
        override
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
    Color3 eval_color_trace(const HitRecord& rec, const Color3& next_color, Color3 brdf, double pdf, const Ray in)
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
