/*
* 相机类
*/
#ifndef CAMERA_H
#define CAMERA_H

#include "color.h"
#include "hittable.h"
#include "image.h"

class Camera
{
public:

    void render(const HitTable& world)
    {
        initialize();

        for (int i = 0; i < image_height_; ++i)
        {
            std::clog << "\rScanlines remaining: " << (image_height_ - i) << ' ' << std::flush;
            for (int j = 0; j < image_width_; ++j)
            {
                // 该像素位置
                auto pixel_center = pixel00_loc_ + (i * pixel_delta_v_) + (j * pixel_delta_u_);
                auto ray_direction = pixel_center - camera_center_;
                // 得到从相机发出的光线
                Ray r(camera_center_, ray_direction);

                Color pixel_color(0, 0, 0);
                for (int sample = 0; sample < samples_per_pixel_; ++sample)
                {
                    Ray r = get_ray(i, j);
                    pixel_color += ray_color(r, world, max_depth_);
                }
                image_->set_pixel(i, j, pixel_color, samples_per_pixel_);
            }
        }
        std::clog << "\rDone.                 \n";
        image_->write();
    }

    void set_image_width(int image_width)
    {
        image_width_ = image_width;
    }

    void set_aspect_ratio(double aspect_ratio)
    {
        aspect_ratio_ = aspect_ratio;
    }

    void set_samples_per_pixel(int samples_per_pixel)
    {
        samples_per_pixel_ = samples_per_pixel;
    }

    void set_max_depth(int max_depth)
    {
        max_depth_ = max_depth;
    }

private:

    double aspect_ratio_ = 1.;
    int    image_width_ = 100;
    int channel_ = 3;
    int    image_height_;
    Point3 camera_center_;
    Point3 pixel00_loc_; // (0,0)处像素的位置
    Vec3   pixel_delta_u_;
    Vec3   pixel_delta_v_;
    // 若使用Image image_，会报错 0xc0000005 访问冲突。
    // 初始化image_使用的临时Image对象会被立刻析构，其持有的image_data_指针在析构函数中释放，
    // image_的image_data_指针是从临时Image对象浅拷贝而来，成为悬空指针，故访问冲突。
    std::unique_ptr<Image> image_;
    int    samples_per_pixel_ = 10; // 每像素采样数
    int    max_depth_ = 10; // 光线最大弹射次数

    // 初始化
    void initialize() 
    {
        image_height_ = static_cast<int>(image_width_ / aspect_ratio_);
        image_height_ = (image_height_ < 1) ? 1 : image_height_;

         //image_ = Image("image.png", image_width_, image_height_, channel_);
        image_ = std::make_unique<Image>("image.png", image_width_, image_height_, channel_);
        
        camera_center_ = Point3(0, 0, 0);

        // 相机属性
        auto focal_length = 1.0;
        auto viewport_height = 2.0;
        auto viewport_width = viewport_height * (static_cast<double>(image_width_) / image_height_);
        // 以左上角为原点，u沿x轴正轴，v沿y轴负轴
        auto viewport_u = Vec3(viewport_width, 0, 0);
        auto viewport_v = Vec3(0, -viewport_height, 0);
        // 每像素对应的视口长度
        pixel_delta_u_ = viewport_u / image_width_;
        pixel_delta_v_ = viewport_v / image_height_;
        // 左上角像素的位置，像素位置以中心点表示
        auto viewport_upper_left = camera_center_ - Vec3(0, 0, focal_length) - viewport_u / 2 - viewport_v / 2;
        pixel00_loc_ = viewport_upper_left + 0.5 * (pixel_delta_u_ + pixel_delta_v_);
    }

    // 获取光线击中处的颜色
    Color ray_color(const Ray& r, const HitTable& world, int depth) const
    {
        if (depth < 0)
        {
            return Color(0, 0, 0);
        }

        HitRecord rec;

        // Interval最小值不能为0，否则当数值误差导致光线与物体交点在物体内部时，光线无法正常弹射
        if (world.hit(r, Interval(0.001, kInfinitDouble), rec)) 
        {
            // 随机在半球上采样方向
            //Vec3 direction = random_on_hemisphere(rec.normal);
            // 朗伯分布，靠近法线的方向上反射可能性更大
            Vec3 direction = rec.normal + random_unit_vector();

            return 0.7 * ray_color(Ray(rec.p, direction), world, depth-1);
        }

        // 背景颜色
        Vec3 unit_direction = unit_vector(r.get_direction());
        // 按y轴蓝白渐变
        auto a = 0.5 * (unit_direction.y() + 1.0);
        return (1.0 - a) * Color(1.0, 1.0, 1.0) + a * Color(0.5, 0.7, 1.0);
    }

    // 采样随机光线
    Ray get_ray(int i, int j) const 
    {
        // 返回长度为1的像素块上一随机采样点位置
        auto pixel_sample_square = [=]() -> Vec3
        {
            auto px = -0.5 + random_double();
            auto py = -0.5 + random_double();
            return (px * pixel_delta_u_) + (py * pixel_delta_v_);
        };

        // 从(i,j)处像素随机采样一条光线
        auto pixel_center = pixel00_loc_ + (j * pixel_delta_u_) + (i * pixel_delta_v_);
        auto pixel_sample = pixel_center + pixel_sample_square();

        auto ray_origin = camera_center_;
        auto ray_direction = pixel_sample - ray_origin;

        return Ray(ray_origin, ray_direction);
    }

};

#endif // !CAMERA_H