/*
 * 相机类
 */
#ifndef CAMERA_H
#define CAMERA_H

#include "image.h"
#include "material.h"
#include "logger.h"

class Camera
{
private:
    // 若使用ImageWrite image_，会报错 0xc0000005 访问冲突。
    // 初始化image_使用的临时ImageWrite对象会被立刻析构，其持有的image_data_指针在析构函数中释放，
    // image_的image_data_指针是从临时ImageWrite对象浅拷贝而来，成为悬空指针，故访问冲突。
    unique_ptr<ImageWrite> image_;
    std::string image_name_;

    double aspect_ratio_;
    double vfov_;  // 垂直fov
    int    image_width_;
    int    image_height_;
    Color  background_;
    int    channel_;
    Point3 camera_center_;
    Point3 pixel00_loc_; // (0,0)处像素的位置
    Vec3   pixel_delta_u_;
    Vec3   pixel_delta_v_;
    int    samples_per_pixel_; // 每像素采样数
    int    sqrt_spp_;
    int    max_depth_; // 光线最大弹射次数

    Point3 lookfrom_;
    Point3 lookat_;
    Vec3   vup_;
    // 相机坐标系
    Vec3   u_; // 指向相机右方
    Vec3   v_; // 指向相机上方
    Vec3   w_; // 与相机视点方向相反 

    double defocus_angle_;  // 光线经过每个像素的变化
    double focus_dist_;    // 相机原点到完美聚焦平面的距离，这里与焦距相同
    Vec3   defocus_disk_u_;  // 散焦横向半径
    Vec3   defocus_disk_v_;  // 散焦纵向半径

public:
    Camera() : 
        aspect_ratio_(1), 
        vfov_(90), 
        background_(.7, .8, 1.), 
        image_height_(0), 
        image_width_(0), 
        channel_(4),
        samples_per_pixel_(16), 
        sqrt_spp_(4),
        max_depth_(10),
        lookfrom_(0, 0, 1), 
        lookat_(0, 0, 0), 
        vup_(0, 1, 0),
        defocus_angle_(0), 
        focus_dist_(10)
    {}

    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;

    Camera(Camera&&) = delete;
    Camera& operator=(Camera&&) = delete;

public:
    // 初始化，并返回渲染图像数组的指针
    unsigned char* initialize()
    {
        image_height_ = static_cast<int>(image_width_ / aspect_ratio_);
        image_height_ = (image_height_ < 1) ? 1 : image_height_;

        image_ = std::make_unique<ImageWrite>(image_name_, image_width_, image_height_, channel_);

        camera_center_ = lookfrom_;

        // 相机属性
        double theta = degrees_to_radians(vfov_);
        double h = tan(theta / 2);
        double viewport_height = 2 * h * focus_dist_;

        double viewport_width = viewport_height * (static_cast<double>(image_width_) / image_height_);

        // 计算相机坐标系，右手系(z轴指向屏幕外)
        w_ = unit_vector(lookfrom_ - lookat_); // 与相机视点方向相反 (0,0,-1)
        u_ = unit_vector(cross(vup_, w_)); // 指向相机右方 (-1,0,0)
        v_ = cross(w_, u_); // 指向相机上方 (0,1,0)

        Vec3 viewport_u = viewport_width * u_;
        Vec3 viewport_v = viewport_height * -v_;

        // 每像素对应的视口长度
        pixel_delta_u_ = viewport_u / image_width_;
        pixel_delta_v_ = viewport_v / image_height_;
        // 左上角像素的位置，像素位置以中心点表示
        auto viewport_upper_left = camera_center_ - (focus_dist_ * w_) - viewport_u / 2 - viewport_v / 2;
        pixel00_loc_ = viewport_upper_left + 0.5 * (pixel_delta_u_ + pixel_delta_v_);

        // 相机散焦平面的基向量
        auto defocus_radius = focus_dist_ * tan(degrees_to_radians(defocus_angle_ / 2));
        defocus_disk_u_ = u_ * defocus_radius;
        defocus_disk_v_ = v_ * defocus_radius;

        return image_->get_image_data();
    }

    // 渲染图像
    void render(const shared_ptr<Hittable>& world, const shared_ptr<Hittable>& light = nullptr)
        const
    {
        rendering.store(true);
        // OpenMP并发
#pragma omp parallel for
        for (int i = 0; i < image_height_; ++i)
        {
            for (int j = 0; j < image_width_; ++j)
            {
                Color pixel_color(0, 0, 0);
                // 对每个像素中的采样点进行分层，采样更均匀
#pragma omp parallel for
                for (int s_i = 0; s_i < sqrt_spp_; ++s_i)
                {
                    for (int s_j = 0; s_j < sqrt_spp_; ++s_j)
                    {
                        if (rendering.load())
                        {
                            Ray r = get_ray(i, j, s_i, s_j);
                            pixel_color += ray_color(r, world, light, max_depth_);
                        }
                    }
                }
                int miss_spp = samples_per_pixel_ - sqrt_spp_ * sqrt_spp_;
                // 补上spp不是完全平方数时漏的采样数
                while (miss_spp--)
                {
                    if (rendering.load())
                    {
                        Ray r = get_ray(i, j, random_int(0, sqrt_spp_), random_int(0, sqrt_spp_));
                        pixel_color += ray_color(r, world, light, max_depth_);
                    }
                }

                image_->set_pixel(i, j, pixel_color, samples_per_pixel_);
            }
        }
        rendering.store(false);
        add_info("Done.");
    }

public:
    void save_image() 
        const
    {
        if(image_->get_image_data() != nullptr)
            image_->write();
    }

    void set_image_width(const int& image_width)
    {
        image_width_ = image_width;
    }

    int get_image_width() 
        const
    {
        return image_width_;
    }

    int get_image_height() 
        const
    {
        return image_height_;
    }

    void set_aspect_ratio(const double& aspect_ratio)
    {
        aspect_ratio_ = aspect_ratio;
    }

    void set_samples_per_pixel(const int& samples_per_pixel)
    {
        samples_per_pixel_ = samples_per_pixel;
        sqrt_spp_ = (int)std::sqrt(samples_per_pixel_);
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
    // 获取光线击中处的颜色
    Color ray_color(const Ray& r_in, const shared_ptr<Hittable>& world, const shared_ptr<Hittable>& light, const int& depth) 
        const
    {
        // 到达弹射次数上限，不再累加任何颜色
        if (depth <= 0)
        {
            return Color(0, 0, 0);
        }

        ++hit_count;

        HitRecord hit_rec; // 击中点记录
        // Interval最小值不能为0，否则当数值误差导致光线与物体交点在物体内部时，光线无法正常弹射
        if (!world->hit(r_in, Interval(1e-3, kInfinitDouble), hit_rec))
            return background_;

        // 只有自发光，无散射
        if (hit_rec.material->no_scatter_)
            return hit_rec.material->eval_color(hit_rec);

        // 下一条散射光线
        Ray r_out = hit_rec.material->sample_ray(r_in, hit_rec);

        // 不用重要性采样
        if (hit_rec.material->skip_pdf_)
            return hit_rec.material->eval_color(hit_rec, ray_color(r_out, world, light, depth - 1));

        double brdf, pdf;
        // 同时对光源和材质采样
        pdf = hit_rec.material->eval_pdf(hit_rec, r_out);
        if (light != nullptr)
        {       
            auto light_pdf = std::make_shared<HittablePDF>(*light, hit_rec.p);

            if (random_double() < 0.5) // 按0.5的概率对光源采样r_out
                r_out = Ray(hit_rec.p, light_pdf->gen_direction(), r_in.get_time());

            pdf = 0.5 * hit_rec.material->eval_pdf(hit_rec, r_out)
                +0.5 * light_pdf->value(r_out.get_direction()); // 按0.5的比例混合pdf值
        }
        brdf = hit_rec.material->eval_brdf(r_in, hit_rec, r_out);

        return hit_rec.material->eval_color(hit_rec, ray_color(r_out, world, light, depth - 1), brdf, pdf);
    }

    // 采样随机光线
    Ray get_ray(int i, int j, int s_i, int s_j) 
        const
    {
        ++sample_count;

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
    Point3 defocus_disk_sample() 
        const 
    {
        auto p = random_in_unit_disk();
        return camera_center_ + (p[0] * defocus_disk_u_) + (p[1] * defocus_disk_v_);
    }

};

#endif // !CAMERA_H