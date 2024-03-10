#include "camera.h"

// 初始化相机，返回图像内存指针
unsigned char** Camera::initialize(bool new_image)
{
    image_height_ = static_cast<int>(image_width_ / aspect_ratio_);
    image_height_ = (image_height_ < 1) ? 1 : image_height_;

    if (new_image)
        image_ = std::make_unique<ImageWrite>(image_name_, image_width_, image_height_, channel_);

    camera_center_ = lookfrom_;

    // 相机属性
    double theta = degrees_to_radians(vfov_);
    double h = tan(theta / 2);
    double viewport_height = 2 * h * focus_dist_;

    double viewport_width = viewport_height * aspect_ratio_;

    // 相机坐标系为右手系，z轴指向屏幕外（与.obj坐标系一致）
    // 
    //     | y v_
    //     |
    //     |
    //    / —————— x u_
    //   /
    //  / z w_
    // 
    w_ = unit_vector(lookfrom_ - lookat_); // 默认 (0,0,1)
    u_ = unit_vector(cross(vup_, w_)); // 默认 (1,0,0)
    v_ = cross(w_, u_); // 默认 (0,1,0)
    w_.normalize();
    u_.normalize();
    v_.normalize();

    Vec3 viewport_u = viewport_width * u_;
    Vec3 viewport_v = viewport_height * -v_; // 视口左上角为相机坐标系原点，因此对v_取负

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

    return image_->get_image_data_p2p();
}

void Camera::trace(const shared_ptr<Hittable>& world, const shared_ptr<Hittable>& light) 
    const
{
    // OpenMP并发
#pragma omp parallel for
    for (int i = 0; i < image_height_; ++i)
    {
        for (int j = 0; j < image_width_; ++j)
        {
            Color3 pixel_color(0, 0, 0);
            // 对每个像素中的采样点进行分层，采样更均匀
#pragma omp parallel for
            for (int s_i = 0; s_i < sqrt_spp_; ++s_i)
            {
                for (int s_j = 0; s_j < sqrt_spp_; ++s_j)
                {
                    if (tracing.load())
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
                if (tracing.load())
                {
                    Ray r = get_ray(i, j, random_int(0, sqrt_spp_), random_int(0, sqrt_spp_));
                    pixel_color += ray_color(r, world, light, max_depth_);
                }
            }
            if (tracing.load())
            {
                image_->set_pixel(i, j, pixel_color, samples_per_pixel_);
            }
        }
    }
    tracing.store(false);
    stop_rastering.store(true);
    add_info("Done.");
}

Color3 Camera::ray_color(const Ray& r_in, const shared_ptr<Hittable>& world, const shared_ptr<Hittable>& light, const int& depth)
    const
{
    // 到达弹射次数上限，不再累加任何颜色
    if (depth <= 0)
    {
        return Color3(0, 0, 0);
    }

    ++hit_count;

    HitRecord hit_rec; // 击中点记录
    // Interval最小值不能为0，否则当数值误差导致光线与物体交点在物体内部时，光线无法正常弹射
    if (!world->hit(r_in, Interval(1e-3, kInfinitDouble), hit_rec))
        return background_;

    // 只有自发光，无散射
    if (hit_rec.material->no_scatter_)
        return hit_rec.material->eval_color_trace(hit_rec);

    // 下一条散射光线
    Ray r_out = hit_rec.material->sample_ray(r_in, hit_rec);

    // 不用重要性采样
    if (hit_rec.material->skip_pdf_)
        return hit_rec.material->eval_color_trace(hit_rec, ray_color(r_out, world, light, depth - 1));

    double pdf;
    Color3 brdf;
    // 同时对光源和材质采样
    pdf = hit_rec.material->eval_pdf(hit_rec.normal, r_out.get_direction(), r_in.get_direction());
    if (light != nullptr)
    {
        auto light_pdf = std::make_shared<HittablePDF>(*light, hit_rec.p);

        if (random_double() < 0.5) // 按0.5的概率对光源采样r_out
            r_out = Ray(hit_rec.p, light_pdf->gen_direction(), r_in.get_time());

        pdf = 0.5 * hit_rec.material->eval_pdf(hit_rec.normal,  r_out.get_direction(), r_in.get_direction())
            + 0.5 * light_pdf->value(r_out.get_direction()); // 按0.5的比例混合pdf值
    }
    brdf = hit_rec.material->eval_brdf(hit_rec.normal, r_out.get_direction(), r_in.get_direction(), hit_rec.u, hit_rec.v);

    return hit_rec.material->eval_color_trace(hit_rec, ray_color(r_out, world, light, depth - 1), brdf, pdf, r_out);
}

Ray Camera::get_ray(int i, int j, int s_i, int s_j)
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

void Camera::rasterize_wireframe(const std::vector<TriangleRasterize>& triangles)
    const
{
    Mat<4, 4> mvp;
    mvp = get_project_matrix() * get_view_matrix();

    Vec3 line_color = { 0,0,0 };

    for (int i = 0; i < triangles.size(); i++)
    {
        // 对顶点进行坐标变换
        TriangleRasterize t = triangles[i];

        t.vertex_[0] = mvp * t.vertex_[0];
        t.vertex_[1] = mvp * t.vertex_[1];
        t.vertex_[2] = mvp * t.vertex_[2];

        // 裁剪        
        if (t.vertex_[0].w() < near_ || t.vertex_[1].w() < near_ || t.vertex_[2].w() < near_ 
            || t.vertex_[0].w() > far_ || t.vertex_[1].w() > far_ || t.vertex_[2].w() > far_)
            continue;
                
        t.vertex_homo_divi(); // 透视除法 
        
        viewport_transformation(t);  // 视口变换

        // 背面三角形使用虚线绘制
        bool back_face = dot(t.get_barycenter_normal(), w_) < 0;
        draw_line(t.vertex_[0], t.vertex_[1], line_color, back_face);
        draw_line(t.vertex_[1], t.vertex_[2], line_color, back_face);
        draw_line(t.vertex_[2], t.vertex_[0], line_color, back_face);
    }
}

void Camera::rasterize_depth(const std::vector<TriangleRasterize>& triangles)
    const
{
    Mat<4, 4> mvp;
    std::vector<std::vector<double>> depth_buf(image_height_, std::vector<double>(image_width_, kInfinitDouble));

    mvp = get_project_matrix() * get_view_matrix();

    double max_depth = -kInfinitDouble;
    double min_depth = kInfinitDouble;

    for (int i = 0; i < triangles.size(); i++)
    {
        TriangleRasterize t = triangles[i];

        // 背面三角形剔除
        if (dot(t.get_barycenter_normal(), w_) < 0)
            continue;

        t.vertex_[0] = mvp * t.vertex_[0];
        t.vertex_[1] = mvp * t.vertex_[1];
        t.vertex_[2] = mvp * t.vertex_[2];

        if (t.vertex_[0].w() < near_ || t.vertex_[1].w() < near_ || t.vertex_[2].w() < near_
            || t.vertex_[0].w() > far_ || t.vertex_[1].w() > far_ || t.vertex_[2].w() > far_)
            continue;

        t.vertex_homo_divi();

        viewport_transformation(t);

        // 覆盖像素
        double maxx = max3(t.vertex_[0].x(), t.vertex_[1].x(), t.vertex_[2].x());
        double minx = min3(t.vertex_[0].x(), t.vertex_[1].x(), t.vertex_[2].x());
        double maxy = max3(t.vertex_[0].y(), t.vertex_[1].y(), t.vertex_[2].y());
        double miny = min3(t.vertex_[0].y(), t.vertex_[1].y(), t.vertex_[2].y());
        // 像素检测
        for (int x = minx; x <= maxx; x++)
        {
            for (int y = miny; y <= maxy; y++)
            {
                if (x < 0 || y < 0 || x >= image_width_ || y >= image_height_)
                    continue;
                if (inside_triangle(t, x, y))
                {
                    double d = interpolated_depth(t, x, y);                    
                    if (d < depth_buf[y][x])
                    {
                        max_depth = std::max(max_depth, d);
                        min_depth = std::min(min_depth, d);
                        depth_buf[y][x] = d;
                    }                        
                }
            }
        }
    }

    //图像绘制
    for (int i = 0; i < image_height_; i++)
    {
        for (int j = 0; j < image_width_; j++)
        {

            if (depth_buf[i][j] == kInfinitDouble)
            {
                image_->set_pixel(i, j, 255, 255, 255);
            }
            else
            {
                double color = (depth_buf[i][j] - min_depth) / (max_depth - min_depth) * 255;
                image_->set_pixel(i, j, color, color, color);
            }
        }
    }
}

void Camera::rasterize_shade(const std::vector<TriangleRasterize>& triangles)
    const
{
    Mat<4, 4> mvp;
    std::vector<std::vector<double>> depth_buf(image_height_, std::vector<double>(image_width_, kInfinitDouble));

    mvp = get_project_matrix() * get_view_matrix();

    for (int i = 0; i < triangles.size(); i++)
    {
        TriangleRasterize t = triangles[i];

        // 背面三角形剔除
        if (dot(t.get_barycenter_normal(), w_) < 0)
            continue;

        t.vertex_[0] = mvp * t.vertex_[0];
        t.vertex_[1] = mvp * t.vertex_[1];
        t.vertex_[2] = mvp * t.vertex_[2];

        if (t.vertex_[0].w() < near_ || t.vertex_[1].w() < near_ || t.vertex_[2].w() < near_
            || t.vertex_[0].w() > far_ || t.vertex_[1].w() > far_ || t.vertex_[2].w() > far_)
            continue;

        t.vertex_homo_divi();

        viewport_transformation(t);

        double maxx = max3(t.vertex_[0].x(), t.vertex_[1].x(), t.vertex_[2].x());
        double minx = min3(t.vertex_[0].x(), t.vertex_[1].x(), t.vertex_[2].x());
        double maxy = max3(t.vertex_[0].y(), t.vertex_[1].y(), t.vertex_[2].y());
        double miny = min3(t.vertex_[0].y(), t.vertex_[1].y(), t.vertex_[2].y());

        for (int x = minx; x <= maxx; x++)
        {
            for (int y = miny; y <= maxy; y++)
            {
                if (x < 0 || y < 0 || x >= image_width_ || y >= image_height_)
                    continue;
                if (inside_triangle(t, x, y))
                {
                    double d = interpolated_depth(t, x, y);
                    // 深度测试
                    if (d < depth_buf[y][x])
                    {
                        image_->set_pixel(y, x, interpolated_material(t, x, y, d));
                        depth_buf[y][x] = d;
                    }                    
                }
            }
        }
    }
}

void Camera::rasterize_coordinate_system()
    const
{
    std::vector<Point4> oxyz{ {0,0,0,1}, {1,0,0,1}, {0,1,0,1}, {0,0,1,1} };
    for (Point4& t : oxyz)
    {
        t = get_project_matrix() * get_view_matrix() * t;
        t /= t.w();
        t.x() = (t.x() + 1.) / 2. * (image_width_  - 1.);
        t.y() = (t.y() + 1.) / 2. * (image_height_ - 1.);
    }

    draw_line(oxyz[0], oxyz[1], { 1, 0, 0 }, false); // x-axis
    draw_line(oxyz[0], oxyz[2], { 0, 1, 0 }, false); // y-axis
    draw_line(oxyz[0], oxyz[3], { 0, 0, 1 }, false); // z-axis
}

Mat<4, 4> Camera::get_view_matrix()
    const
{
    Mat<4, 4> view;

    view =
    { {
        { u_.x(),  u_.y(),  u_.z(), -dot( u_, lookfrom_)},
        {-v_.x(), -v_.y(), -v_.z(), -dot(-v_, lookfrom_)},
        { w_.x(),  w_.y(),  w_.z(), -dot( w_, lookfrom_)},
        {0, 0, 0, 1}
    } };

    return view;
}

Mat<4, 4> Camera::get_project_matrix()
    const
{
    Mat<4, 4> project;

    project =
    { {
        {1 / (tan(degrees_to_radians(vfov_) / 2) * aspect_ratio_), 0, 0, 0},
        {0, 1 / tan(degrees_to_radians(vfov_) / 2), 0, 0},
        {0, 0, (near_ + far_) / (near_ - far_), 2 * near_ * far_ / (near_ - far_)},
        {0, 0, -1, 0}
    } };
    return project;
}

void Camera::draw_line(const Point4& a, const Point4& b, const Color3& color, bool dotted)
    const
{
    double x_a = a[0];
    double x_b = b[0];
    double y_a = a[1];
    double y_b = b[1];

    bool steep = false;
    if (std::abs(x_a - x_b) < std::abs(y_a - y_b))
    {
        std::swap(x_a, y_a);
        std::swap(x_b, y_b);
        steep = true;
    }
    if (x_a > x_b)
    {
        std::swap(x_a, x_b);
        std::swap(y_a, y_b);
    }

    int interval = dotted ? 2 : 1;
    for (int x_ = x_a; x_ <= x_b; x_ += interval)
    {
        double t  = (x_ - x_a) / (x_b - x_a);
        int    y_ = y_a * (1. - t) + y_b * t;
        if (x_ < 0 || y_ < 0) 
            continue;
        if (steep)
        {
            if (x_ > image_height_ - 1 || y_ > image_width_ - 1)
                continue;
            image_->set_pixel(x_, y_, color);
        }
        else
        {
            // set_pixel输入为row,col，因此未调换时输入y_,x_
            if (y_ > image_height_ - 1 || x_ > image_width_ - 1)   
                continue;
            image_->set_pixel(y_, x_, color);
        }
    }
}

void Camera::viewport_transformation(TriangleRasterize& triangle)
    const
{
    for (auto& v : triangle.vertex_)
    {
        // NDC坐标范围为[-1,1]，变换到[0,1]再缩放
        v.x() = (v.x() + 1.) / 2. * (image_width_  - 1.);
        v.y() = (v.y() + 1.) / 2. * (image_height_ - 1.);
        // 视口变换不需要改变z
    }
}

bool Camera::inside_triangle(const TriangleRasterize& triangle, const double& x, const double& y)
    const
{
    Vec3 AB(triangle.vertex_[1] - triangle.vertex_[0]);
    Vec3 CA(triangle.vertex_[0] - triangle.vertex_[2]);
    Vec3 BC(triangle.vertex_[2] - triangle.vertex_[1]);

    Point3 Q = Point3(x, y, 0);
    Vec3 AQ(Q - Point3(triangle.vertex_[0]));
    Vec3 CQ(Q - Point3(triangle.vertex_[2]));
    Vec3 BQ(Q - Point3(triangle.vertex_[1]));

    double ABAQ = cross(AB, AQ).z();
    double CACQ = cross(CA, CQ).z();
    double BCBQ = cross(BC, BQ).z();

    if ((ABAQ >= 0 && CACQ >= 0 && BCBQ >= 0) || (ABAQ <= 0 && CACQ <= 0 && BCBQ <= 0))
        return true;
    else
        return false;
}

// 插值算法参考：https://zhuanlan.zhihu.com/p/448575965
double Camera::interpolated_depth(const TriangleRasterize& triangle, const double& x, const double& y)
    const
{
    auto [alpha, beta, gamma] = barycentric_coordinate(triangle, x, y);

    // 由于透视变换，屏幕空间下插值为相机空间下的倒数的插值
    double depth = 1. / 
        (alpha / triangle.vertex_[0].z() 
            + beta / triangle.vertex_[1].z() 
            + gamma / triangle.vertex_[2].z());
    return depth;
}

Color3 Camera::interpolated_material(const TriangleRasterize& triangle, const double& x, const double& y, const double& depth)
    const
{
    auto [alpha, beta, gamma] = barycentric_coordinate(triangle, x, y);

    // 不和深度一样直接对倒数插值，而是借助深度间接插值，避免除0产生inf影响结果
    Texcoord2 uv = depth * 
        (alpha / triangle.vertex_[0].z() * triangle.texcoord_[0]
            + beta / triangle.vertex_[1].z()  * triangle.texcoord_[1]
            + gamma / triangle.vertex_[2].z() * triangle.texcoord_[2]);

    Vec3 normal = depth *
        (alpha / triangle.vertex_[0].z() * triangle.normal_[0]
            + beta / triangle.vertex_[1].z() * triangle.normal_[1]
            + gamma / triangle.vertex_[2].z() * triangle.normal_[2]);

    // HACK 恒定方向光
    return triangle.material_->eval_color_rasterize(uv, normal, Color3(15, 15, 15), background_ * .1, Vec3(0, 1, 0));
}

std::tuple<double, double, double> Camera::barycentric_coordinate(const TriangleRasterize& triangle, const double& x, const double& y)
    const
{
    auto v = triangle.vertex_;

    double alpha =
        (x * (v[1].y() - v[2].y()) + (v[2].x() - v[1].x()) * y + v[1].x() * v[2].y() - v[2].x() * v[1].y())
        / (v[0].x() * (v[1].y() - v[2].y()) + (v[2].x() - v[1].x()) * v[0].y() + v[1].x() * v[2].y() - v[2].x() * v[1].y());
    double beta =
        (x * (v[2].y() - v[0].y()) + (v[0].x() - v[2].x()) * y + v[2].x() * v[0].y() - v[0].x() * v[2].y())
        / (v[1].x() * (v[2].y() - v[0].y()) + (v[0].x() - v[2].x()) * v[1].y() + v[2].x() * v[0].y() - v[0].x() * v[2].y());
    double gamma = 1 - alpha - beta;

    return { alpha, beta, gamma };
}