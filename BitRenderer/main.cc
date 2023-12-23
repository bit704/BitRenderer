#include "image.h"
#include "sphere.h"
#include "hittable_list.h"

Color ray_color(const Ray& r)
{
	static HittableList hittablelist(make_shared<Sphere>(Point3(0, 0, -1), 0.5));
    HitRecord hr;
    if (hittablelist.hit(r, 0.1, 1000, hr)) // 如果光线击中物体，返回最近击中点
    {
        // 击中处法线
        Vec3 N = hr.normal;
        // 将法线向量每个值从[-1,1]缩放到[0,1]，供颜色用
        return 0.5 * Color(N.x() + 1, N.y() + 1, N.z() + 1);
    }

    // 背景颜色
    Vec3 unit_direction = unit_vector(r.get_direction());
    // 按y轴蓝白渐变
    auto a = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - a) * Color(1.0, 1.0, 1.0) + a * Color(0.5, 0.7, 1.0);
}

int main() 
{
    // 图像
    auto aspect_ratio = 16.0 / 9.0;
    int image_width = 512;
    int image_height = static_cast<int>(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;
    int channel = 3;

    // 相机
    auto focal_length = 1.0;
    auto viewport_height = 2.0;
    auto viewport_width = viewport_height * (static_cast<double>(image_width) / image_height);
    auto camera_center = Point3(0, 0, 0);
    // 视口属性原点
    // 以左上角为原点，u沿x轴正轴，v沿y轴负轴
    auto viewport_u = Vec3(viewport_width, 0, 0);
    auto viewport_v = Vec3(0, -viewport_height, 0);
    // 每像素对应的视口长度
    auto pixel_delta_u = viewport_u / image_width;
    auto pixel_delta_v = viewport_v / image_height;
    // 左上角像素的位置，像素位置以中心点表示
    auto viewport_upper_left = camera_center - Vec3(0, 0, focal_length) - viewport_u / 2 - viewport_v / 2;
    auto pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);


    std::unique_ptr<Image> image(new Image("image.png", image_width, image_height, channel));

    for (int i = 0; i < image_height; ++i) 
    {
        std::clog << "\rScanlines remaining: " << (image_height - i) << ' ' << std::flush;
        for (int j = 0; j < image_width; ++j) 
        {
            // 该像素位置
            auto pixel_center = pixel00_loc + (i * pixel_delta_v) + (j * pixel_delta_u);
            auto ray_direction = pixel_center - camera_center;
            // 得到从相机发出的光线
            Ray r(camera_center, ray_direction);

            image->set_pixel(i, j, ray_color(r));
        }
    }
    std::clog << "\rDone.                 \n";

    image->write();
    return 0;
}