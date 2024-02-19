/*
 * 相机类
 */
#ifndef CAMERA_H
#define CAMERA_H

#include "image.h"
#include "material.h"
#include "logger.h"
#include "mat.h"
#include "triangle_rasterize.h"

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
    Color3  background_;
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
        vfov_(20),         
        image_height_(0), 
        image_width_(0), 
        channel_(4),
        samples_per_pixel_(16), 
        sqrt_spp_(4),
        max_depth_(10),
        lookfrom_(0, 0, 1), 
        lookat_(0, 0, 0), 
        vup_(0, 1, 0),
        background_(1, 1, 1),
        defocus_angle_(0), 
        focus_dist_(10)
    {}

    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;

    Camera(Camera&&) = delete;
    Camera& operator=(Camera&&) = delete;

/*
 * 公用 
 */
public:
    // 初始化，并返回渲染图像数组的指针的指针
    unsigned char** initialize(bool new_image);
    
    void clear()
        const
    {
        image_->flush(background_);
    }

/*
 * 光线追踪 
 */
public:
    // 光线追踪渲染图像
    void trace(const shared_ptr<Hittable>& world, const shared_ptr<Hittable>& light = nullptr)
        const;

private:
    // 获取光线击中处的颜色
    Color3 ray_color(const Ray& r_in, const shared_ptr<Hittable>& world, const shared_ptr<Hittable>& light, const int& depth)
        const;

    // 采样随机光线
    Ray get_ray(int i, int j, int s_i, int s_j)
        const;

    // 返回圆形透镜上随机一点
    Point3 defocus_disk_sample() 
        const 
    {
        auto p = random_in_unit_disk();
        return camera_center_ + (p[0] * defocus_disk_u_) + (p[1] * defocus_disk_v_);
    }

/*
 * 光栅化 
 */
public:
    // 光栅化渲染图像
    void rasterize(const std::vector<TriangleRasterize>& triangles, const int& mode)
        const
    {
        if (mode == 0)
            rasterize_wireframe(triangles);
        else if (mode == 1)
            rasterize_depth(triangles);
        else if (mode == 2)
            rasterize_shade(triangles);
        return;       
    }

private:
    void rasterize_wireframe(const std::vector<TriangleRasterize>& triangles)
        const;

    void rasterize_depth(const std::vector<TriangleRasterize>& triangles)
        const;

    void rasterize_shade(const std::vector<TriangleRasterize>& triangles)
        const;

    //返回视图矩阵
    Mat<4, 4> get_view_matrix()
        const;

    //返回投影矩阵
    Mat<4, 4> get_project_matrix()
        const;

    void draw_line(const Point4& a, const Point4& b, const Color3& color, bool dotted = false)
        const;

    void viewport_transformation(TriangleRasterize& triangle)
        const;

    bool inside_triangle(TriangleRasterize triangle, double x, double y)
        const;

    double interpolated_depth(TriangleRasterize t, double x, double y)
        const;

/*
 * getter/setter
 */
public:
    void save_image()
        const
    {
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

    double get_aspect_ratio()
        const
    {
        return aspect_ratio_;
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

    double get_vfov()
        const
    {
        return vfov_;
    }

    void set_lookfrom(const Vec3& lookfrom)
    {
        lookfrom_ = lookfrom;
    }

    Vec3 get_lookfrom()
        const
    {
        return lookfrom_;
    }

    void set_lookat(const Vec3& lookat)
    {
        lookat_ = lookat;
    }

    Vec3 get_lookat()
        const
    {
        return lookat_;
    }

    void set_vup(const Vec3& vup)
    {
        vup_ = vup;
    }

    Vec3 get_vup()
        const
    {
        return vup_;
    }

    void set_defocus_angle(const double& defocus_angle)
    {
        defocus_angle_ = defocus_angle;
    }

    void set_focus_dist(const double& focus_dist)
    {
        focus_dist_ = focus_dist;
    }

    void set_background(const Color3& background)
    {
        background_ = background;
    }

    Color3 get_background()
        const
    {
        return background_;
    }

    void set_image_name(const std::string& image_name)
    {
        image_name_ = image_name;
    }

/*
 * 键鼠交互
 */
public:
    void move_front_back(double v)
    {
        lookfrom_ = lookfrom_ - w_ * v;
    }

    void move_left_right(double v)
    {

    }

    void move_up_down(double v)
    {

    }

    void zoom_fov(double v)
    {

    }

    void view_third_person(double x, double y)
    {

    }

    void view_first_person(double x, double y)
    {

    }
};

#endif // !CAMERA_H