#include "image.h"
#include "sphere.h"
#include "hittable_list.h"
#include "camera.h"

int main() 
{
    HittableList hittablelist;
    hittablelist.add(std::make_shared<Sphere>(Point3(0, -100.5, -1), 100));
    hittablelist.add(std::make_shared<Sphere>(Point3(0, 0, -1), .5));

    Camera cam;
    cam.set_aspect_ratio(16. / 9.);
    cam.set_image_width(256);
    cam.set_samples_per_pixel(100);
    cam.set_max_depth(50);
    cam.render(hittablelist);

    return 0;
}