#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "scene.h"
#include "logger.h"
#include "rst_tri.h"

std::vector<Point3> vertices;
std::vector<Point3> normals;
std::vector<std::pair<double, double>> texcoords;

void scene_obj_rasterize(Camera& cam, const fs::path& obj_path, const int rst_mode)
{
    // TODO 从obj_path为光栅化渲染加载三角形网格
    // TODO 光栅化渲染图像，参数未定 
    std::vector<Rst_Tri> triangles;
    if (!load_obj_rst(obj_path.string().c_str(), obj_path.parent_path().string().c_str(), true, triangles))
    {
        add_info(obj_path.string() + "  failed to load.");
        return;
    }
    cam.rasterize(triangles);
    return;
}


void scene_obj_trace(const Camera& cam, const fs::path& obj_path)
{
    shared_ptr<HittableList> world = make_shared<HittableList>();
    HittableList triangles;

    auto start = steady_clock::now();
    if (!load_obj_hittable(obj_path.string().c_str(), obj_path.parent_path().string().c_str(), true, triangles))
    {
        add_info(obj_path.string() + "  failed to load.");
        return;
    }
    auto end = steady_clock::now();
    add_info("loading elapsed time: "_str + STR(duration_cast<milliseconds>(end - start).count()) + "ms");

    add_info("prepare BVH...");
    start = steady_clock::now();
    world->add(make_shared<BVHNode>(triangles));
    end = steady_clock::now();
    add_info("BVH elapsed time: "_str + STR(duration_cast<milliseconds>(end - start).count()) + "ms");

    cam.trace(world);
    return;
}

// 测试三角形
void scene_test_triangle(const Camera& cam)
{
    shared_ptr<HittableList> world = make_shared<HittableList>();

    vertices = { {0, 0, 1}, {1, 0, 0}, {0, 1, 0} };
    normals = { {0, 0, 1}, {0, 0, 1}, {0, 0, 1} };
    texcoords = { {0, 0}, {1, 0}, {0, 1} };
    auto earthmap = make_shared<ImageTexture>("earthmap.jpg");
    world->add(make_shared<Triangle>(0, 0, 0, 1, 1, 1, 2, 2, 2, make_shared<Lambertian>(earthmap)));

    cam.trace(world);
    return;
}


// 为光线追踪加载三角形网格
bool load_obj_hittable(const char* filename, const char* basepath, bool triangulate, HittableList& triangles)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
        filename, basepath, triangulate);

    add_info("load .obj: "_str + filename);

    if (!warn.empty())
    {
        LOG("WARN: ", warn);
    }

    if (!err.empty())
    {
        LOG("ERR: ", err);
    }

    if (!ret)
    {
        LOG("Failed to load/parse .obj.\n");
        return false;
    }

    ullong vnum = attrib.vertices.size()  / 3;
    ullong nnum = attrib.normals.size()   / 3;
    ullong tnum = attrib.texcoords.size() / 2;
    ullong snum = shapes.size();
    ullong mnum = materials.size();

    add_info("vertices: "_str  + STR(vnum));
    add_info("normals: "_str   + STR(nnum));
    add_info("texcoords: "_str + STR(tnum));
    add_info("shapes: "_str    + STR(snum));
    add_info("materials: "_str + STR(mnum));

    vertices  = std::vector<Point3>(vnum);
    normals   = std::vector<Vec3>(nnum);
    texcoords = std::vector<std::pair<double, double>>(tnum);

    for (ullong v = 0; v < vnum; ++v)
    {
        vertices[v] = Point3(
            static_cast<const double>(attrib.vertices[3 * v + 0]),
            static_cast<const double>(attrib.vertices[3 * v + 1]),
            static_cast<const double>(attrib.vertices[3 * v + 2]));
    }

    for (ullong n = 0; n < nnum; ++n)
    {
        normals[n] = Vec3(
            static_cast<const double>(attrib.normals[3 * n + 0]),
            static_cast<const double>(attrib.normals[3 * n + 1]),
            static_cast<const double>(attrib.normals[3 * n + 2]));
    }

    for (ullong t = 0; t < tnum; ++t)
    {
        texcoords[t] = std::pair<double, double>(
            static_cast<const double>(attrib.texcoords[2 * t + 0]),
            static_cast<const double>(attrib.texcoords[2 * t + 1]));
    }

    //auto white = make_shared<Lambertian>(Color(.73, .73, .73));
    auto white = make_shared<Lambertian>(make_shared<ImageTexture>("earthmap.jpg"));
    
    ullong tot_fnum = 0; // 总面数
    for (ullong i = 0; i < snum; i++)
    {
        tot_fnum += shapes[i].mesh.num_face_vertices.size();
    }
    std::vector<shared_ptr<Hittable>> tris = std::vector<shared_ptr<Hittable>>(tot_fnum);
    ullong tri_index = 0;

    for (ullong i = 0; i < snum; i++)
    {
        add_info("shape name: "_str + shapes[i].name);
        add_info("  mesh indices num: "_str + STR(shapes[i].mesh.indices.size()));
        add_info("  lines indices num: "_str + STR(shapes[i].lines.indices.size()));
        add_info("  points indices num: "_str + STR(shapes[i].points.indices.size()));
        
        ullong fnum = shapes[i].mesh.num_face_vertices.size();

        assert(fnum == shapes[i].mesh.material_ids.size());
        assert(fnum == shapes[i].mesh.smoothing_group_ids.size());

        add_info("  faces num: "_str + STR(fnum));

        ullong index_offset = 0;
        for (ullong f = 0; f < fnum; f++)
        {
            ullong v_per_f = shapes[i].mesh.num_face_vertices[f];
            // 每面顶点数必须为3
            assert(v_per_f == 3);

            tinyobj::index_t idx_a = shapes[i].mesh.indices[index_offset];
            int a  = idx_a.vertex_index;
            int an = idx_a.normal_index;
            int at = idx_a.texcoord_index;

            tinyobj::index_t idx_b = shapes[i].mesh.indices[index_offset + 1];
            int b  = idx_b.vertex_index;
            int bn = idx_b.normal_index;
            int bt = idx_b.texcoord_index;

            tinyobj::index_t idx_c = shapes[i].mesh.indices[index_offset + 2];
            int c  = idx_c.vertex_index;
            int cn = idx_c.normal_index;
            int ct = idx_c.texcoord_index;

            tris[tri_index + f] = make_shared<Triangle>(a, an, at, b, bn, bt, c, cn, ct, white);

            index_offset += v_per_f;
        }
        tri_index += fnum;
    }

    triangles = std::move(tris);

    return true;
}
bool load_obj_rst(const char* filename, const char* basepath, bool triangulate, std::vector<Rst_Tri>& triangles)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
        filename, basepath, triangulate);

    

    ullong vnum = attrib.vertices.size() / 3;
    ullong nnum = attrib.normals.size() / 3;
    ullong tnum = attrib.texcoords.size() / 2;
    ullong snum = shapes.size();
    ullong mnum = materials.size();

    vertices = std::vector<Point3>(vnum);
    normals = std::vector<Vec3>(nnum);
    texcoords = std::vector<std::pair<double, double>>(tnum);

    for (ullong v = 0; v < vnum; ++v)
    {
        vertices[v] = Point3(
            static_cast<const double>(attrib.vertices[3 * v + 0]),
            static_cast<const double>(attrib.vertices[3 * v + 1]),
            static_cast<const double>(attrib.vertices[3 * v + 2]));
    }

    for (ullong n = 0; n < nnum; ++n)
    {
        normals[n] = Vec3(
            static_cast<const double>(attrib.normals[3 * n + 0]),
            static_cast<const double>(attrib.normals[3 * n + 1]),
            static_cast<const double>(attrib.normals[3 * n + 2]));
    }

    for (ullong t = 0; t < tnum; ++t)
    {
        texcoords[t] = std::pair<double, double>(
            static_cast<const double>(attrib.texcoords[2 * t + 0]),
            static_cast<const double>(attrib.texcoords[2 * t + 1]));
    }

    //auto white = make_shared<Lambertian>(Color(.73, .73, .73));
    auto white = make_shared<Lambertian>(make_shared<ImageTexture>("earthmap.jpg"));

    ullong tot_fnum = 0; // 总面数
    for (ullong i = 0; i < snum; i++)
    {
        tot_fnum += shapes[i].mesh.num_face_vertices.size();
    }
    std::vector<Rst_Tri> tris = std::vector<Rst_Tri>(tot_fnum);
    ullong tri_index = 0;

    for (ullong i = 0; i < snum; i++)
    {
        ullong fnum = shapes[i].mesh.num_face_vertices.size();

        assert(fnum == shapes[i].mesh.material_ids.size());
        assert(fnum == shapes[i].mesh.smoothing_group_ids.size());

        ullong index_offset = 0;
        for (ullong f = 0; f < fnum; f++)
        {
            ullong v_per_f = shapes[i].mesh.num_face_vertices[f];
            // 每面顶点数必须为3
            assert(v_per_f == 3);

            tinyobj::index_t idx_a = shapes[i].mesh.indices[index_offset];
            int a = idx_a.vertex_index;
            int an = idx_a.normal_index;
            int at = idx_a.texcoord_index;

            tinyobj::index_t idx_b = shapes[i].mesh.indices[index_offset + 1];
            int b = idx_b.vertex_index;
            int bn = idx_b.normal_index;
            int bt = idx_b.texcoord_index;

            tinyobj::index_t idx_c = shapes[i].mesh.indices[index_offset + 2];
            int c = idx_c.vertex_index;
            int cn = idx_c.normal_index;
            int ct = idx_c.texcoord_index;

            //根据索引存储三角形
            Rst_Tri tri;
            tri.set_vertex(vertices[a], vertices[b], vertices[c]);
            tri.set_normal(normals[an], normals[bn], normals[cn]);
            tri.set_texcoord(texcoords[at], texcoords[bt], texcoords[ct]);


            tris[tri_index + f] = tri;

            index_offset += v_per_f;
        }
        tri_index += fnum;
    }

    triangles = tris;

    return true;
}

void scene_checker(const Camera& cam)
{
    shared_ptr<HittableList> world = make_shared<HittableList>();

    auto checker = make_shared<CheckerTexture>(0.8, Color(.2, .3, .1), Color(.9, .9, .9));

    world->add(make_shared<Sphere>(Point3(0, -10, 0), 10, make_shared<Lambertian>(checker)));
    world->add(make_shared<Sphere>(Point3(0, 10, 0), 10, make_shared<Lambertian>(checker)));

    cam.trace(world);
    return;
}

void scene_cornell_box(const Camera& cam)
{
    shared_ptr<HittableList> world = make_shared<HittableList>();

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

    cam.trace(world, light);
    return;
}

void scene_composite1(const Camera& cam)
{
    shared_ptr<HittableList> world = make_shared<HittableList>();
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

    cam.trace(world);
    return;
}

void scene_composite2(const Camera& cam)
{
    shared_ptr<HittableList> world = make_shared<HittableList>();

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

    cam.trace(world, light);
    return;
}