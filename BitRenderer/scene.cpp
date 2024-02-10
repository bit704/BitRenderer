#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "scene.h"
#include "logger.h"
#include "triangle_rasterize.h"

std::vector<Point3>    vertices;
std::vector<Vec3>      normals;
std::vector<Texcoord2> texcoords;

void scene_obj_rasterize(const Camera& cam, const fs::path& obj_path, const int& mode)
{
    static std::vector<TriangleRasterize> triangles;
    static fs::path prev_obj_path;

    // 避免每帧重复加载模型
    if (prev_obj_path != obj_path)
    {
        prev_obj_path = obj_path;
        if (!load_obj_rasterize(obj_path.string().c_str(), obj_path.parent_path().string().c_str(), true, triangles))
        {
            add_info(obj_path.string() + "  failed to load for rastering.");
            return;
        }
    }

    cam.rasterize(triangles, mode);
    return;
}

void scene_obj_trace(const Camera& cam, const fs::path& obj_path)
{
    shared_ptr<HittableList> world = make_shared<HittableList>();
    HittableList triangles;

    auto start = steady_clock::now();
    if (!load_obj_trace(obj_path.string().c_str(), obj_path.parent_path().string().c_str(), true, triangles))
    {
        add_info(obj_path.string() + "  failed to load for ray tracing.");
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

static tinyobj::attrib_t attrib;
static std::vector<tinyobj::shape_t> shapes;
static std::vector<tinyobj::material_t> materials;
static ullong vnum = 0, nnum = 0, tnum = 0, snum = 0, mnum = 0, tot_fnum = 0;

bool load_obj_internal(const char* filename, const char* basepath, bool triangulate)
{
    std::string warn;
    std::string err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
        filename, basepath, triangulate);

    add_info("load .obj: "_str + filename);

    if (!warn.empty())
    {
        add_info("WARN: " + warn);
    }

    if (!err.empty())
    {
        add_info("ERR: " + err);
    }

    if (!ret)
    {
        add_info("Failed to load/parse .obj.");
        return false;
    }

    vnum = attrib.vertices.size() / 3;
    nnum = attrib.normals.size() / 3;
    tnum = attrib.texcoords.size() / 2;
    snum = shapes.size();
    mnum = materials.size();

    add_info("vertices: "  + STR(vnum));
    add_info("normals: "   + STR(nnum));
    add_info("texcoords: " + STR(tnum));
    add_info("shapes: "    + STR(snum));
    add_info("materials: " + STR(mnum));

    vertices = std::vector<Point3>(vnum);
    normals = std::vector<Vec3>(nnum);
    texcoords = std::vector<Texcoord2>(tnum);

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
        texcoords[t] = Texcoord2(
            static_cast<const double>(attrib.texcoords[2 * t + 0]),
            static_cast<const double>(attrib.texcoords[2 * t + 1]));
    }

    for (ullong i = 0; i < snum; i++)
    {
        tot_fnum += shapes[i].mesh.num_face_vertices.size();
    }

    return true;
}

// 为光线追踪加载三角形网格
bool load_obj_trace(const char* filename, const char* basepath, bool triangulate, HittableList& triangles)
{
    // 不必再次加载obj，光栅化已加载

    //auto white = make_shared<Lambertian>(Color3(.73, .73, .73));
    auto white = make_shared<Lambertian>(make_shared<ImageTexture>("earthmap.jpg"));

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

// 为光栅化加载三角形网格
bool load_obj_rasterize(const char* filename, const char* basepath, bool triangulate, std::vector<TriangleRasterize>& triangles)
{
    if (!load_obj_internal(filename, basepath, triangulate))
    {
        return false;
    }

    std::vector<TriangleRasterize> tris = std::vector<TriangleRasterize>(tot_fnum);
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

            //根据索引存储三角形
            TriangleRasterize tri;
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

    auto checker = make_shared<CheckerTexture>(0.8, Color3(.2, .3, .1), Color3(.9, .9, .9));

    world->add(make_shared<Sphere>(Point3(0, -10, 0), 10, make_shared<Lambertian>(checker)));
    world->add(make_shared<Sphere>(Point3(0, 10, 0), 10, make_shared<Lambertian>(checker)));

    cam.trace(world);
    return;
}

void scene_cornell_box(const Camera& cam)
{
    shared_ptr<HittableList> world = make_shared<HittableList>();

    // 材质
    auto red = make_shared<Lambertian>(Color3(.65, .05, .05));
    auto white = make_shared<Lambertian>(Color3(.73, .73, .73));
    auto green = make_shared<Lambertian>(Color3(.12, .45, .15));
    auto lighting = make_shared<DiffuseLight>(Color3(15, 15, 15));
    auto aluminum = make_shared<Metal>(Color3(.8, .85, .88), 0.);

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

    auto ground_material = make_shared<Lambertian>(Color3(0.5, 0.5, 0.5));
    list.add(make_shared<Sphere>(Point3(0, -1000, 0), 1000, ground_material));

    for (int a = -11; a < 11; ++a)
    {
        for (int b = -11; b < 11; ++b)
        {
            auto choose_mat = random_double();
            Point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());

            if ((center - Point3(4, 0.2, 0)).norm() > 0.9)
            {
                shared_ptr<Material> sphere_material;

                if (choose_mat < 0.8)
                {
                    auto albedo = Color3::random() * Color3::random();
                    sphere_material = make_shared<Lambertian>(albedo);
                    // 在0~1时间内从center运动到center_end，随机向上弹跳
                    //auto center_end = center + Vec3(0, random_double(0, .5), 0);
                    //list.add(make_shared<Sphere>(center, center_end, 0.2, sphere_material));
                    list.add(make_shared<Sphere>(center, 0.2, sphere_material));
                }
                else if (choose_mat < 0.95)
                {
                    auto albedo = Color3::random(0.5, 1);
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
    auto material2 = make_shared<Lambertian>(Color3(0.4, 0.2, 0.1));
    list.add(make_shared<Sphere>(Point3(-4, 1, 0), 1.0, material2));
    // 高光
    auto material3 = make_shared<Metal>(Color3(0.7, 0.6, 0.5), 0.0);
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
    auto ground_mat = make_shared<Lambertian>(Color3(0.48, 0.83, 0.53));
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
    auto sphere_mat = make_shared<Lambertian>(Color3(0.7, 0.3, 0.1));
    world->add(make_shared<Sphere>(center1, center2, 50, sphere_mat));

    // 玻璃球
    world->add(make_shared<Sphere>(Point3(260, 150, 45), 50, make_shared<Dielectric>(1.5)));
    // 金属球
    world->add(make_shared<Sphere>(Point3(0, 150, 145), 50, make_shared<Metal>(Color3(0.8, 0.8, 0.9), 1.0)));

    // 内含蓝色雾效的玻璃球
    auto boundary = make_shared<Sphere>(Point3(360, 150, 145), 70, make_shared<Dielectric>(1.5));
    world->add(boundary);
    world->add(make_shared<ConstantMedium>(boundary, 0.2, Color3(0.2, 0.4, 0.9)));

    // 全场景雾效
    boundary = make_shared<Sphere>(Point3(0, 0, 0), 5000, make_shared<Dielectric>(1.5));
    world->add(make_shared<ConstantMedium>(boundary, .0001, Color3(1, 1, 1)));

    // 地球
    auto earthmap = make_shared<Lambertian>(make_shared<ImageTexture>("earthmap.jpg"));
    world->add(make_shared<Sphere>(Point3(400, 200, 400), 100, earthmap));

    // 柏林噪声球
    auto pertext = make_shared<NoiseTexture>(0.1);
    world->add(make_shared<Sphere>(Point3(220, 280, 300), 80, make_shared<Lambertian>(pertext)));

    // 球组成的立方体
    HittableList boxes2;
    auto white = make_shared<Lambertian>(Color3(.73, .73, .73));
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
    auto lighting = make_shared<DiffuseLight>(Color3(7, 7, 7));
    world->add(make_shared<Quad>(Point3(123, 554, 147), Vec3(300, 0, 0), Vec3(0, 0, 265), lighting));

    // 对光源几何体采样
    shared_ptr<HittableList> light(new HittableList());
    auto m = shared_ptr<Material>();
    light->add(make_shared<Quad>(Point3(123, 554, 147), Vec3(300, 0, 0), Vec3(0, 0, 265), m));

    cam.trace(world, light);
    return;
}