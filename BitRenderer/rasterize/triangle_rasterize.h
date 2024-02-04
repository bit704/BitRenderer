/*
 *  光栅化三角形类
 */
#ifndef TRIANGLE_RASTERIZE_H
#define TRIANGLE_RASTERIZE_H

#include "vec.h"

class TriangleRasterize
{
public:
    Point4    vertex_[3];
    Vec3      normal_[3];
    Texcoord2 texcoord_[3];

    TriangleRasterize()
    {
        // 点的w坐标为1
        vertex_[0][3] = 1;
        vertex_[1][3] = 1;
        vertex_[2][3] = 1;
    }

    void set_vertex(Point3 a, Point3 b, Point3 c)
    {
        vertex_[0] = { a.x(), a.y(), a.z() , 1};
        vertex_[1] = { b.x(), b.y(), b.z() , 1};
        vertex_[2] = { c.x(), c.y(), c.z() , 1};
    }

    void set_normal(Vec3 a, Vec3 b, Vec3 c)
    {
        normal_[0] = { a.x(), a.y(), a.z() };
        normal_[1] = { b.x(), b.y(), b.z() };
        normal_[2] = { c.x(), c.y(), c.z() };
    }

    void set_texcoord(Texcoord2 a, Texcoord2 b, Texcoord2 c)
    {
        texcoord_[0] = { a.u(), a.v() };
        texcoord_[1] = { b.u(), b.v() };
        texcoord_[2] = { c.u(), c.v() };
    }

    void vertex_homo_divi()
    {
        vertex_[0] /= vertex_[0][3];
        vertex_[1] /= vertex_[1][3];
        vertex_[2] /= vertex_[2][3];
    }
};
#endif // !TRIANGLE_RASTERIZE_H