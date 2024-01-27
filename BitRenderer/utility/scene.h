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

void scene_test_triangle(const Camera& cam);

bool load_obj(const char* filename, const char* basepath, bool triangulate, HittableList& triangles);

// 加载obj的场景
void scene_obj(const Camera& cam, const fs::path& obj_path);

// 3D棋盘格纹理，两个球
void scene_checker(const Camera& cam);

// Cornell Box 1984
void scene_cornell_box(const Camera& cam);

void scene_composite1(const Camera& cam);

void scene_composite2(const Camera& cam);

#endif // !SCENE_H

