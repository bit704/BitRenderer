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
extern std::vector<Texcoord2> texcoords;

void scene_test_triangle(const Camera& cam);

// 光栅化实时渲染场景 
bool prepare_rasterize_data(const char* filename, const char* basepath, bool triangulate, std::vector<TriangleRasterize>& triangles);
void scene_rasterize(const Camera& cam, const fs::path& obj_path, const shared_ptr<Material>& material, const int& mode);

// 光线追踪离线渲染场景
bool prepare_trace_data(HittableList& triangles, const shared_ptr<Material>& material);
void scene_trace(const Camera& cam, const fs::path& obj_path, const shared_ptr<Material>& material, const bool& tracing_with_cornell_box);

// 3D棋盘格纹理，两个球
void scene_checker(const Camera& cam);

// Cornell Box 1984
void scene_cornell_box(const Camera& cam);

void scene_composite1(const Camera& cam);

void scene_composite2(const Camera& cam);

#endif // !SCENE_H

