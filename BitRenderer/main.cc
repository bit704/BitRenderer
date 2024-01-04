#include <chrono>
#include <string>
#include <thread>

#include "image.h"
#include "logger.h"
#include "scene.h"

using std::chrono::steady_clock;
using std::chrono::system_clock;
using std::chrono::duration_cast;
using std::chrono::minutes;
using std::chrono::seconds;

// 计算次数统计
int cal_count = 0;

int main()
{
    auto start = steady_clock::now();

    const int all = ((1LL << 31) - 1);
    int onoff = 1 << 0;

    Camera cam;
    unsigned char* image_data = nullptr;
    std::thread t;

    if (1 << 0 & onoff) 
    {
        cam.set_image_name("scene_composite1.png");
        cam.set_aspect_ratio(16. / 9.);
        cam.set_image_width(400);
        cam.set_samples_per_pixel(100);
        cam.set_max_depth(50);
        cam.set_vfov(20);
        cam.set_lookfrom(Point3(13, 2, 3));
        cam.set_lookat(Point3(0, 0, 0));
        cam.set_vup(Vec3(0, 1, 0));
        cam.set_defocus_angle(0.6);

        image_data = cam.initialize();
        t = std::thread(scene_composite1, std::ref(cam));
    }
    if (1 << 1 & onoff)  
    {
        cam.set_image_name("scene_checker.png");
        cam.set_aspect_ratio(16. / 9.);
        cam.set_image_width(400);
        cam.set_samples_per_pixel(100);
        cam.set_max_depth(50);
        cam.set_vfov(20);
        cam.set_lookfrom(Point3(13, 2, 3));
        cam.set_lookat(Point3(0, 0, 0));
        cam.set_vup(Vec3(0, 1, 0));
        cam.set_defocus_angle(0.);

        image_data = cam.initialize();
        t = std::thread(scene_checker, std::ref(cam));
    }
    if (1 << 2 & onoff)
    {
        cam.set_image_name("scene_cornell_box.png");
        cam.set_aspect_ratio(1);
        cam.set_image_width(600);
        cam.set_samples_per_pixel(100);
        cam.set_max_depth(50);
        cam.set_vfov(40);
        cam.set_lookfrom(Point3(278, 278, -800));
        cam.set_lookat(Point3(278, 278, 0));
        cam.set_vup(Vec3(0, 1, 0));
        cam.set_defocus_angle(0.);
        cam.set_background(Color(0., 0., 0.));

        image_data = cam.initialize();
        t = std::thread(scene_cornell_box, std::ref(cam));
    }
    if (1 << 3 & onoff)
    {
        cam.set_image_name("scene_composite2.png");
        cam.set_aspect_ratio(1);
        cam.set_image_width(400);
        cam.set_samples_per_pixel(300);
        cam.set_max_depth(40);
        cam.set_vfov(40);
        cam.set_lookfrom(Point3(478, 278, -600));
        cam.set_lookat(Point3(278, 278, 0));
        cam.set_vup(Vec3(0, 1, 0));
        cam.set_background(Color(0., 0., 0.));
        cam.set_defocus_angle(0.);

        image_data = cam.initialize();
        t = std::thread(scene_composite2, std::ref(cam));
    }

    t.join();

    auto end = steady_clock::now();

    LOG("总计算量：", std::to_string(cal_count));

    auto duration = end - start;
    auto duration_min = duration_cast<minutes>(duration);
    auto duration_sec = duration_cast<seconds>(duration);

    LOG("总计算时长：", std::to_string(duration_min.count()), "分钟");
    LOG("计算速度：", std::to_string(cal_count / duration_sec.count()), "次/秒");

    return 0;
}
