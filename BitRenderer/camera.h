/*
* 相机类
*/
#ifndef CAMERA_H
#define CAMERA_H

#include <cmath>
#include <iomanip>

#include "color.h"
#include "hittable.h"
#include "image.h"
#include "material.h"
#include "pdf.h"
#include "logger.h"

extern int cal_count;

class Camera
{
public:

    void render(const std::shared_ptr<Hittable>& world, const std::shared_ptr<Hittable>& light = nullptr)
    {
        initialize();

        sqrt_spp_ = (int)std::sqrt(samples_per_pixel_);

        for (int i = 0; i < height_; ++i)
        {
            std::clog << std::fixed << std::setprecision(2);
            std::clog << "\rtask remaining: " << (double)(height_ - i - 1) / height_ * 100 << "%" << std::flush;

#pragma omp parallel for

            for (int j = 0; j < width_; ++j)
            {
                Color pixel_color(0, 0, 0);
                // 对每个像素中的采样点进行分层，采样更均匀
                for (int s_i = 0; s_i < sqrt_spp_; ++s_i)
                {
                    for (int s_j = 0; s_j < sqrt_spp_; ++s_j)
                    {
                        Ray r = get_ray(i, j, s_i, s_j);
                        pixel_color += ray_color(r, world, light, max_depth_);
                    }
                }
                image_->set_pixel(i, j, pixel_color, samples_per_pixel_);
            }
        }
        std::clog << "\rDone.                 \n";
        image_->write();
    }

    void set_image_width(const int& image_width)
    {
        width_ = image_width;
    }

    void set_aspect_ratio(const double& aspect_ratio)
    {
        aspect_ratio_ = aspect_ratio;
    }

    void set_samples_per_pixel(const int& samples_per_pixel)
    {
        samples_per_pixel_ = samples_per_pixel;
    }

    void set_max_depth(const int& max_depth)
    {
        max_depth_ = max_depth;
    }

    void set_vfov(const double& vfov)
    {
        vfov_ = vfov;
    }

    void set_lookfrom(const Vec3& lookfrom)
    {
        lookfrom_ = lookfrom;
    }

    void set_lookat(const Vec3& lookat)
    {
        lookat_ = lookat;
    }

    void set_vup(const Vec3& vup)
    {
        vup_ = vup;
    }

    void set_defocus_angle(const double& defocus_angle)
    {
        defocus_angle_ = defocus_angle;
    }

    void set_focus_dist(const double& focus_dist)
    {
        focus_dist_ = focus_dist;
    }

    void set_background(const Color& background)
    {
        background_ = background;
    }

    void set_image_name(const std::string& image_name)
    {
        image_name_ = image_name;
    }

private:

    // 若使用ImageWrite image_，会报错 0xc0000005 访问冲突。
    // 初始化image_使用的临时ImageWrite对象会被立刻析构，其持有的image_data_指针在析构函数中释放，
    // image_的image_data_指针是从临时ImageWrite对象浅拷贝而来，成为悬空指针，故访问冲突。
    std::unique_ptr<ImageWrite> image_;
    std::string image_name_;

    double aspect_ratio_ = 1.;
    int    width_ = 100;
    int    channel_ = 3;
    int    height_;
    Point3 camera_center_;
    Point3 pixel00_loc_; // (0,0)处像素的位置
    Vec3   pixel_delta_u_;
    Vec3   pixel_delta_v_;
    int    samples_per_pixel_ = 16; // 每像素采样数
    int    sqrt_spp_ = 4;
    int    max_depth_ = 10; // 光线最大弹射次数

    double vfov_ = 90;  // 垂直fov
    
    Point3 lookfrom_ = Point3(0, 0, -1);
    Point3 lookat_ = Point3(0, 0, 0);
    Vec3   vup_ = Point3(0, 1, 0);
    // 相机坐标系
    Vec3   u_; // 指向相机右方
    Vec3   v_; // 指向相机上方
    Vec3   w_; // 与相机视点方向相反 

    double defocus_angle_ = 0;  // 光线经过每个像素的变化
    double focus_dist_ = 10;    // 相机原点到完美聚焦平面的距离，这里与焦距相同
    Vec3 defocus_disk_u_;  // 散焦横向半径
    Vec3 defocus_disk_v_;  // 散焦纵向半径

    Color background_ = Color(.7, .8, 1.);

    // 初始化
    void initialize() 
    {
        height_ = static_cast<int>(width_ / aspect_ratio_);
        height_ = (height_ < 1) ? 1 : height_;

        image_ = std::make_unique<ImageWrite>(image_name_, width_, height_, channel_);
        
        camera_center_ = lookfrom_;

        // 相机属性
        auto theta = degrees_to_radians(vfov_);
        auto h = tan(theta / 2);
        auto viewport_height = 2 * h * focus_dist_;

        auto viewport_width = viewport_height * (static_cast<double>(width_) / height_);
        
        // 计算相机坐标系，右手系(z轴指向屏幕外)
        w_ = unit_vector(lookfrom_ - lookat_); // 与相机视点方向相反 (0,0,-1)
        u_ = unit_vector(cross(vup_, w_)); // 指向相机右方 (-1,0,0)
        v_ = cross(w_, u_); // 指向相机上方 (0,1,0)

        Vec3 viewport_u = viewport_width * u_;
        Vec3 viewport_v = viewport_height * -v_;

        // 每像素对应的视口长度
        pixel_delta_u_ = viewport_u / width_;
        pixel_delta_v_ = viewport_v / height_;
        // 左上角像素的位置，像素位置以中心点表示
        auto viewport_upper_left = camera_center_ - (focus_dist_ * w_) - viewport_u / 2 - viewport_v / 2;
        pixel00_loc_ = viewport_upper_left + 0.5 * (pixel_delta_u_ + pixel_delta_v_);

        // 相机散焦平面的基向量
        auto defocus_radius = focus_dist_ * tan(degrees_to_radians(defocus_angle_ / 2));
        defocus_disk_u_ = u_ * defocus_radius;
        defocus_disk_v_ = v_ * defocus_radius;
    }

    // 获取光线击中处的颜色
    Color ray_color(const Ray& r, const std::shared_ptr<Hittable>& world, const std::shared_ptr<Hittable>& light, int depth) const
    {
        // 到达弹射次数上限，不再累加任何颜色
        if (depth < 0)
        {
            return Color(0, 0, 0);
        }

        HitRecord rec;

        // Interval最小值不能为0，否则当数值误差导致光线与物体交点在物体内部时，光线无法正常弹射
        if (!world->hit(r, Interval(1e-3, kInfinitDouble), rec))
            return background_;

        ++cal_count;

        ScatterRecord srec;

        Color color_from_emission = rec.material->emitted(rec.u, rec.v, rec.p); // 自发光颜色

        // 只有自发光颜色
        if (!rec.material->scatter(r, rec, srec))
            return color_from_emission;

        if (srec.skip_pdf)
        {
            return srec.attenuation * ray_color(srec.skip_pdf_ray, world, light, depth - 1);
        }

        Ray scattered;
        double pdf_val;
        // 有无光源采样
        if (light != nullptr)
        {
            auto light_ptr = std::make_shared<HittablePDF>(*light, rec.p);
            MixturePDF p(light_ptr, srec.pdf_ptr);

            scattered = Ray(rec.p, p.generate(), r.get_time());
            pdf_val = p.value(scattered.get_direction());
        }
        else
        {
            scattered = Ray(rec.p, srec.pdf_ptr->generate(), r.get_time());
            pdf_val = srec.pdf_ptr->value(scattered.get_direction());
        }

        double scattering_pdf = rec.material->scattering_pdf(r, rec, scattered);

        Color sample_color = ray_color(scattered, world, light, depth - 1);
        Color color_from_scatter = (srec.attenuation * scattering_pdf * sample_color) / pdf_val;

        return color_from_scatter + color_from_emission;
    }

    // 采样随机光线
    Ray get_ray(int i, int j, int s_i, int s_j) const
    {
        // 返回长度为1的像素块上一随机采样点位置
        auto pixel_sample_square = [=](int s_i, int s_j) -> Vec3
        {
            auto px = -0.5 + 1. / sqrt_spp_ * (s_i + random_double());
            auto py = -0.5 + 1. / sqrt_spp_ * (s_j + random_double());
            return (px * pixel_delta_u_) + (py * pixel_delta_v_);
        };

        // 从(i,j)处像素随机采样一条光线
        auto pixel_center = pixel00_loc_ + (j * pixel_delta_u_) + (i * pixel_delta_v_);
        auto pixel_sample = pixel_center + pixel_sample_square(s_i, s_j);

        // auto ray_origin = camera_center_;
        // 散焦，在圆形透镜上随机采样，光线原点不再是相机原点
        auto ray_origin = (defocus_angle_ <= 0) ? camera_center_ : defocus_disk_sample();
        auto ray_direction = pixel_sample - ray_origin;

        // 在[0,1)时间间隔之间随机生成光线
        auto ray_time = random_double();

        return Ray(ray_origin, ray_direction, ray_time);
    }

    // 返回圆形透镜上随机一点
    Point3 defocus_disk_sample() const 
    {
        auto p = random_in_unit_disk();
        return camera_center_ + (p[0] * defocus_disk_u_) + (p[1] * defocus_disk_v_);
    }

};

#endif // !CAMERA_H