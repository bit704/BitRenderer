#include "image.h"
#include "sphere.h"
#include "hittable_list.h"
#include "camera.h"

int main() 
{
    HittableList hittablelist;
    
    auto material_ground = std::make_shared<Lambertian>(Color(0.8, 0.8, 0.));
    auto material_center = std::make_shared<Lambertian>(Color(0.7, 0.5, 0.5));
    auto material_left = std::make_shared<Metal>(Color(0.8, 0.8, 0.8), 0.3);
    auto material_right = std::make_shared<Metal>(Color(0.8, 0.6, 0.2), 1.);

    hittablelist.add(std::make_shared<Sphere>(Point3(0., -100.5, -1.), 100., material_ground));
    hittablelist.add(std::make_shared<Sphere>(Point3(0., 0., -1.), 0.5, material_center));
    hittablelist.add(std::make_shared<Sphere>(Point3(-1., 0., -1.0), 0.5, material_left));
    hittablelist.add(std::make_shared<Sphere>(Point3(1., 0., -1.), 0.5, material_right));

    Camera cam;
    cam.set_aspect_ratio(16. / 9.);
    cam.set_image_width(256);
    cam.set_samples_per_pixel(100);
    cam.set_max_depth(50);
    cam.render(hittablelist);

    return 0;
}