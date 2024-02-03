/*
 *  光栅化三角形类
 */
#ifndef TRIANGLE_RASTERIZE_H
#define TRIANGLE_RASTERIZE_H

#include "geometry.h"

class TriangleRasterize
{
public:
    vec4 vertex_[3];
    vec3 normal_[3];
    vec2 texcoord_[3];

    TriangleRasterize()
    {
        vertex_[0] = { 0, 0, 0 ,1 };
        vertex_[1] = { 0, 0, 0 ,1 };
        vertex_[2] = { 0, 0, 0 ,1 };

        normal_[0] = { 0, 0, 0 };
        normal_[1] = { 0, 0, 0 };
        normal_[2] = { 0, 0, 0 };

        texcoord_[0] = { 0,0 };
        texcoord_[1] = { 0,0 };
        texcoord_[2] = { 0,0 };
    }

    void set_vertex(Point3 a, Point3 b, Point3 c)
    {
        vertex_[0][0] = a.x();
        vertex_[0][1] = a.y();
        vertex_[0][2] = a.z();

        vertex_[1][0] = b.x();
        vertex_[1][1] = b.y();
        vertex_[1][2] = b.z();

        vertex_[2][0] = c.x();
        vertex_[2][1] = c.y();
        vertex_[2][2] = c.z();

    }

    void set_normal(Vec3 a, Vec3 b, Vec3 c)
    {
        normal_[0].x = a.x();
        normal_[0].y = a.y();
        normal_[0].z = a.z();

        normal_[1].x = b.x();
        normal_[1].y = b.y();
        normal_[1].z = b.z();

        normal_[2].x = c.x();
        normal_[2].y = c.y();
        normal_[2].z = c.z();
    }

    void set_texcoord(std::pair<double, double> a, std::pair<double, double> b, std::pair<double, double> c)
    {
        texcoord_[0].x = a.first;
        texcoord_[0].y = a.second;

        texcoord_[1].x = b.first;
        texcoord_[1].y = b.second;

        texcoord_[2].x = c.first;
        texcoord_[2].y = c.second;
    }

    void vertex_homo_divi()
    {
        vertex_[0][0] /= vertex_[0][3];
        vertex_[0][1] /= vertex_[0][3];
        vertex_[0][2] /= vertex_[0][3];
        vertex_[0][3] /= vertex_[0][3];

        vertex_[1][0] /= vertex_[1][3];
        vertex_[1][1] /= vertex_[1][3];
        vertex_[1][2] /= vertex_[1][3];
        vertex_[1][3] /= vertex_[1][3];

        vertex_[2][0] /= vertex_[2][3];
        vertex_[2][1] /= vertex_[2][3];
        vertex_[2][2] /= vertex_[2][3];
        vertex_[2][3] /= vertex_[2][3];
    }
};
#endif // !TRIANGLE_RASTERIZE_H