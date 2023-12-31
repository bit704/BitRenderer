#include <chrono>
#include <string>
#include "image.h"
#include "sphere.h"
#include "hittable_list.h"
#include "camera.h"
#include "logger.h"
#include "bvh_node.h"

using std::make_shared;
using std::shared_ptr;
using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::minutes;
using std::chrono::seconds;

int cal_count = 0;

// 多球弹跳
void scene_1()
{
    HittableList world;

    auto ground_material = make_shared<Lambertian>(Color(0.5, 0.5, 0.5));
    world.add(make_shared<Sphere>(Point3(0, -1000, 0), 1000, ground_material));

    for (int a = -11; a < 11; ++a)
    {
        for (int b = -11; b < 11; ++b)
        {
            auto choose_mat = random_double();
            Point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());

            if ((center - Point3(4, 0.2, 0)).length() > 0.9)
            {
                shared_ptr<Material> sphere_material;

                if (choose_mat < 0.8)
                {
                    auto albedo = Color::random() * Color::random();
                    sphere_material = make_shared<Lambertian>(albedo);
                    // 在0~1时间内从center运动到center_end，向上的弹跳效果
                    auto center_end = center + Vec3(0, random_double(0, .5), 0);
                    world.add(make_shared<Sphere>(center, center_end, 0.2, sphere_material));
                }
                else if (choose_mat < 0.95)
                {
                    auto albedo = Color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<Metal>(albedo, fuzz);
                    world.add(make_shared<Sphere>(center, 0.2, sphere_material));
                }
                else
                {
                    sphere_material = make_shared<Dielectric>(1.5);
                    world.add(make_shared<Sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }
    // 折射
    auto material1 = make_shared<Dielectric>(1.5);
    world.add(make_shared<Sphere>(Point3(0, 1, 0), 1.0, material1));
    // 漫反射
    auto material2 = make_shared<Lambertian>(Color(0.4, 0.2, 0.1));
    world.add(make_shared<Sphere>(Point3(-4, 1, 0), 1.0, material2));
    // 高光
    auto material3 = make_shared<Metal>(Color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<Sphere>(Point3(4, 1, 0), 1.0, material3));

    // BVH加速结构
    auto world_bvh = BVHNode(world);

    Camera cam;
    cam.set_aspect_ratio(16. / 9.);
    cam.set_image_width(400);
    cam.set_samples_per_pixel(100);
    cam.set_max_depth(50);

    cam.set_vfov(20);
    cam.set_lookfrom(Point3(13, 2, 3));
    cam.set_lookat(Point3(0, 0, 0));
    cam.set_vup(Vec3(0, 1, 0));

    cam.set_defocus_angle(0.6);
    cam.set_focus_dist(10.);

    cam.render(world_bvh);
}

// 地球
void scene_2()
{
    auto earth_texture = make_shared<ImageTexture>("../texture/earthmap.jpg");
    auto earth_surface = make_shared<Lambertian>(earth_texture);
    auto globe = Sphere(Point3(0, 0, 0), 2, earth_surface);

    Camera cam;
    cam.set_aspect_ratio(16. / 9.);
    cam.set_image_width(400);
    cam.set_samples_per_pixel(100);
    cam.set_max_depth(50);

    cam.set_vfov(20);
    cam.set_lookfrom(Point3(0, 0, 12));
    cam.set_lookat(Point3(0, 0, 0));
    cam.set_vup(Vec3(0, 1, 0));

    cam.set_defocus_angle(0.);
    cam.set_focus_dist(10.);

    cam.render(globe);
}

// 两个柏林噪声球
void scene_3()
{
    HittableList world;

    auto pertext = make_shared<NoiseTexture>();
    world.add(make_shared<Sphere>(Point3(0, -1000, 0), 1000, make_shared<Lambertian>(pertext)));
    world.add(make_shared<Sphere>(Point3(0, 2, 0), 2, make_shared<Lambertian>(pertext)));

    Camera cam;
    cam.set_aspect_ratio(16. / 9.);
    cam.set_image_width(400);
    cam.set_samples_per_pixel(100);
    cam.set_max_depth(50);

    cam.set_vfov(20);
    cam.set_lookfrom(Point3(13, 2, 3));
    cam.set_lookat(Point3(0, 0, 0));
    cam.set_vup(Vec3(0, 1, 0));

    cam.set_defocus_angle(0.);
    cam.set_focus_dist(10.);

    cam.render(world);
}


int main() 
{
    auto start = steady_clock::now();

    switch (3)
    {
        case 1: scene_1(); break;
        case 2: scene_2(); break;
        case 3: scene_3(); break;
    }

    auto end = steady_clock::now();

    LOG("总计算量：" + std::to_string(cal_count));

    auto duration = end - start;
    auto duration_min = duration_cast<minutes>(duration);
    auto duration_sec = duration_cast<seconds>(duration);

    LOG("总计算时长：" + std::to_string(duration_min.count()) + "分钟");
    LOG("计算速度：" + std::to_string(cal_count / duration_sec.count()) + "次/秒");

    return 0;
}