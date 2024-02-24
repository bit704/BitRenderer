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
    shared_ptr<Material> material_;

    TriangleRasterize()
    {
        // 点的w坐标为1
        vertex_[0].w() = 1;
        vertex_[1].w() = 1;
        vertex_[2].w() = 1;
    }

    void set_material(shared_ptr<Material> material)
    {
        material_ = material;
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
        // 保留w不变
        vertex_[0].x() /= vertex_[0].w();
        vertex_[0].y() /= vertex_[0].w();
        vertex_[0].z() /= vertex_[0].w();
        vertex_[1].x() /= vertex_[1].w();
        vertex_[1].y() /= vertex_[1].w();
        vertex_[1].z() /= vertex_[1].w();
        vertex_[2].x() /= vertex_[2].w();
        vertex_[2].y() /= vertex_[2].w();
        vertex_[2].z() /= vertex_[2].w();
    }

    Vec3 get_barycenter_normal()
    {
        return normal_[0] / 3 + normal_[1] / 3 + normal_[2] / 3;
    }
};
#endif // !TRIANGLE_RASTERIZE_H