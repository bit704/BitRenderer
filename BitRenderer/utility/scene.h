/*
 * 预置场景函数
 */
#ifndef SCENE_H
#define SCENE_H

#include "bvh_node.h"
#include "camera.h"
#include "constant_medium.h"
#include "quad.h"
#include "sphere.h"
#include "triangle.h"

extern std::vector<Point3> vertices;
extern std::vector<Point3> normals;
extern std::vector<std::pair<double, double>> texcoords;

inline void scene_obj(const Camera& cam, const std::string& obj_path)
{
    shared_ptr<HittableList> world(new HittableList());

    vertices = { {0, 0, 0}, {1, 0, 0}, {0, 1, 0} };
    normals = { {0, 0, 1}, {0, 0, 1}, {0, 0, 1} };
    texcoords = { {0, 0}, {1, 0}, {0, 1} };
    auto earthmap = make_shared<ImageTexture>("earthmap.jpg");
    world->add(make_shared<Triangle>(0, 0, 0, 1, 1, 1, 2, 2, 2, make_shared<Lambertian>(earthmap)));

    cam.render(world);
}

// 3D棋盘格纹理，两个球
inline void scene_checker(const Camera& cam)
{
    shared_ptr<HittableList> world(new HittableList());

    auto checker = make_shared<CheckerTexture>(0.8, Color(.2, .3, .1), Color(.9, .9, .9));

    world->add(make_shared<Sphere>(Point3(0, -10, 0), 10, make_shared<Lambertian>(checker)));
    world->add(make_shared<Sphere>(Point3(0, 10, 0), 10, make_shared<Lambertian>(checker)));

    cam.render(world);
}

// Cornell Box 1984
inline void scene_cornell_box(const Camera& cam)
{
    shared_ptr<HittableList> world(new HittableList());

    // 材质
    auto red = make_shared<Lambertian>(Color(.65, .05, .05));
    auto white = make_shared<Lambertian>(Color(.73, .73, .73));
    auto green = make_shared<Lambertian>(Color(.12, .45, .15));
    auto lighting = make_shared<DiffuseLight>(Color(15, 15, 15));
    auto aluminum = make_shared<Metal>(Color(.8, .85, .88), 0.);

    // 包围盒体
    world->add(make_shared<Quad>(Point3(555, 0, 0), Vec3(0, 555, 0), Vec3(0, 0, 555), green));
    world->add(make_shared<Quad>(Point3(0, 0, 0), Vec3(0, 555, 0), Vec3(0, 0, 555), red));
    world->add(make_shared<Quad>(Point3(0, 0, 0), Vec3(555, 0, 0), Vec3(0, 0, 555), white));
    world->add(make_shared<Quad>(Point3(555, 555, 555), Vec3(-555, 0, 0), Vec3(0, 0, -555), white));
    world->add(make_shared<Quad>(Point3(0, 0, 555), Vec3(555, 0, 0), Vec3(0, 555, 0), white));

    // 长方体
    shared_ptr<Hittable> box1 = construct_box(Point3(0, 0, 0), Point3(165, 180, 165), aluminum);
    box1 = make_shared<RotateY>(box1, 45);
    box1 = make_shared<Translate>(box1, Vec3(240, 0, 240));
    world->add(box1);

    // 玻璃球
    auto glass = make_shared<Dielectric>(1.5);
    world->add(make_shared<Sphere>(Point3(180, 160, 190), 90, glass));

    // 光源
    world->add(make_shared<Quad>(Point3(343, 554, 332), Vec3(-130, 0, 0), Vec3(0, 0, -105), lighting));

    // 对光源几何体采样
    shared_ptr<HittableList> light(new HittableList());
    auto m = shared_ptr<Material>();
    light->add(make_shared<Quad>(Point3(343, 554, 332), Vec3(-130, 0, 0), Vec3(0, 0, -105), m));
    // 对玻璃球采样
    light->add(make_shared<Sphere>(Point3(190, 90, 190), 90, m));

    cam.render(world, light);
}

inline void scene_composite1(const Camera& cam)
{
    shared_ptr<HittableList> world(new HittableList());
    HittableList list;

    auto ground_material = make_shared<Lambertian>(Color(0.5, 0.5, 0.5));
    list.add(make_shared<Sphere>(Point3(0, -1000, 0), 1000, ground_material));

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
                    // 在0~1时间内从center运动到center_end，随机向上弹跳
                    //auto center_end = center + Vec3(0, random_double(0, .5), 0);
                    //list.add(make_shared<Sphere>(center, center_end, 0.2, sphere_material));
                    list.add(make_shared<Sphere>(center, 0.2, sphere_material));
                }
                else if (choose_mat < 0.95)
                {
                    auto albedo = Color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<Metal>(albedo, fuzz);
                    list.add(make_shared<Sphere>(center, 0.2, sphere_material));
                }
                else
                {
                    sphere_material = make_shared<Dielectric>(1.5);
                    list.add(make_shared<Sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }
    // 折射
    auto material1 = make_shared<Dielectric>(1.5);
    list.add(make_shared<Sphere>(Point3(0, 1, 0), 1.0, material1));
    // 漫反射
    auto material2 = make_shared<Lambertian>(Color(0.4, 0.2, 0.1));
    list.add(make_shared<Sphere>(Point3(-4, 1, 0), 1.0, material2));
    // 高光
    auto material3 = make_shared<Metal>(Color(0.7, 0.6, 0.5), 0.0);
    list.add(make_shared<Sphere>(Point3(4, 1, 0), 1.0, material3));

    // BVH加速结构
    world->add(make_shared<BVHNode>(list));

    cam.render(world);
}

inline void scene_composite2(const Camera& cam)
{
    shared_ptr<HittableList> world(new HittableList());

    // 立方体组成的地板
    HittableList boxes1;
    auto ground_mat = make_shared<Lambertian>(Color(0.48, 0.83, 0.53));
    int boxes_per_side = 20;
    for (int i = 0; i < boxes_per_side; ++i)
    {
        for (int j = 0; j < boxes_per_side; ++j)
        {
            auto w = 100.;
            auto x0 = -1000. + i * w;
            auto z0 = -1000. + j * w;
            auto y0 = 0.;
            auto x1 = x0 + w;
            auto y1 = random_double(1, 101);
            auto z1 = z0 + w;

            boxes1.add(construct_box(Point3(x0, y0, z0), Point3(x1, y1, z1), ground_mat));
        }
    }
    world->add(make_shared<BVHNode>(boxes1));

    // 运动球
    auto center1 = Point3(400, 400, 200);
    auto center2 = center1 + Vec3(30, 0, 0);
    auto sphere_mat = make_shared<Lambertian>(Color(0.7, 0.3, 0.1));
    world->add(make_shared<Sphere>(center1, center2, 50, sphere_mat));

    // 玻璃球
    world->add(make_shared<Sphere>(Point3(260, 150, 45), 50, make_shared<Dielectric>(1.5)));
    // 金属球
    world->add(make_shared<Sphere>(Point3(0, 150, 145), 50, make_shared<Metal>(Color(0.8, 0.8, 0.9), 1.0)));

    // 内含蓝色雾效的玻璃球
    auto boundary = make_shared<Sphere>(Point3(360, 150, 145), 70, make_shared<Dielectric>(1.5));
    world->add(boundary);
    world->add(make_shared<ConstantMedium>(boundary, 0.2, Color(0.2, 0.4, 0.9)));

    // 全场景雾效
    boundary = make_shared<Sphere>(Point3(0, 0, 0), 5000, make_shared<Dielectric>(1.5));
    world->add(make_shared<ConstantMedium>(boundary, .0001, Color(1, 1, 1)));

    // 地球
    auto earthmap = make_shared<Lambertian>(make_shared<ImageTexture>("earthmap.jpg"));
    world->add(make_shared<Sphere>(Point3(400, 200, 400), 100, earthmap));

    // 柏林噪声球
    auto pertext = make_shared<NoiseTexture>(0.1);
    world->add(make_shared<Sphere>(Point3(220, 280, 300), 80, make_shared<Lambertian>(pertext)));

    // 球组成的立方体
    HittableList boxes2;
    auto white = make_shared<Lambertian>(Color(.73, .73, .73));
    int ns = 1000;
    for (int j = 0; j < ns; j++)
    {
        boxes2.add(make_shared<Sphere>(Point3::random(0, 165), 10, white));
    }
    world->add(
        make_shared<Translate>(
            make_shared<RotateY>(make_shared<BVHNode>(boxes2), 15),
            Vec3(-100, 270, 395))
    );

    // 光源
    auto lighting = make_shared<DiffuseLight>(Color(7, 7, 7));
    world->add(make_shared<Quad>(Point3(123, 554, 147), Vec3(300, 0, 0), Vec3(0, 0, 265), lighting));

    // 对光源几何体采样
    shared_ptr<HittableList> light(new HittableList());
    auto m = shared_ptr<Material>();
    light->add(make_shared<Quad>(Point3(123, 554, 147), Vec3(300, 0, 0), Vec3(0, 0, 265), m));

    cam.render(world, light);
}

#endif // !SCENE_H

