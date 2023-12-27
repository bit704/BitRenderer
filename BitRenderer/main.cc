#include "image.h"
#include "sphere.h"
#include "hittable_list.h"
#include "camera.h"

using std::make_shared;

int main() 
{
    HittableList hittablelist;
    
    auto material_ground = make_shared<Lambertian>(Color(0.8, 0.8, 0.0));
    auto material_center = make_shared<Lambertian>(Color(0.1, 0.2, 0.5));
    auto material_left = make_shared<Dielectric>(1.5);
    auto material_right = make_shared<Metal>(Color(0.8, 0.6, 0.2), 0.0);

    hittablelist.add(make_shared<Sphere>(Point3(0., -100.5, -1.), 100., material_ground));
    hittablelist.add(make_shared<Sphere>(Point3(0., 0., -1.), 0.5, material_center));
    hittablelist.add(make_shared<Sphere>(Point3(-1., 0., -1.0), 0.5, material_left));
    hittablelist.add(make_shared<Sphere>(Point3(-1.0, 0.0, -1.0), -0.4, material_left)); // 负半径，与上一个组成空心玻璃球
    hittablelist.add(make_shared<Sphere>(Point3(1., 0., -1.), 0.5, material_right));

    Camera cam;
    cam.set_aspect_ratio(16. / 9.);
    cam.set_image_width(256);
    cam.set_samples_per_pixel(100);
    cam.set_max_depth(50);

    cam.set_vfov(30);
    cam.set_lookfrom(Point3(-2, 2, 1));
    cam.set_lookat(Point3(0, 0, -1));
    cam.set_vup(Vec3(0, 1, 0));

    cam.render(hittablelist);

    return 0;
}