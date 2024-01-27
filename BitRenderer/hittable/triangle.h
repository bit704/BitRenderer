/*
 * 三角形类
 */
#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "common.h"
#include "hittable.h"

extern std::vector<Point3> vertices;
extern std::vector<Point3> normals;
extern std::vector<std::pair<double, double>> texcoords;

class Triangle : public Hittable
{
private:
    uint a_,  b_,  c_;  // 顶点索引
    uint an_, bn_, cn_; // 法线索引
    uint at_, bt_, ct_; // 纹理坐标索引
    shared_ptr<Material> material_;
    AABB bbox_;

public:
    Triangle(
        uint a, uint an, uint at, uint b, uint bn, uint bt, uint c, uint cn, uint ct,
        shared_ptr<Material> material)
        : a_(a), an_(an), at_(at), b_(b), bn_(bn), bt_(bt), c_(c), cn_(cn), ct_(ct),
        material_(material)
    {
        bbox_ = AABB(AABB(vertices[a_], vertices[b_]).pad(), AABB(vertices[a_], vertices[c_]).pad());
    }

    Triangle(const Triangle&) = delete;
    Triangle& operator=(const Triangle&) = delete;

    Triangle(Triangle&&) = delete;
    Triangle& operator=(Triangle&&) = delete;

public:
    // 参考
    // https://dl.acm.org/doi/10.1145/1198555.1198746
    // https://www.cnblogs.com/graphics/archive/2010/08/09/1795348.html

    bool hit(const Ray& r, const Interval& interval, HitRecord& rec)
        const override
    {
        Vec3 e1 = vertices[b_] - vertices[a_];
        Vec3 e2 = vertices[c_] - vertices[a_];
        Vec3 p = cross(r.get_direction(), e2);
        double det = dot(e1, p);

        Vec3 t;
        if (det > 0)
        {
            t = r.get_origin() - vertices[a_];
        }
        else
        {
            t = vertices[a_] - r.get_origin();
            det = -det;
        }

        // 光线与三角形接近平行
        if (det < 1e-4)
            return false;

        // 重心坐标
        double bc1, bc2;
        bc1 = dot(t, p);
        if (bc1 < 0.f || bc1 > det)
            return false;

        Vec3 q = cross(t, e1);
        bc2 = dot(r.get_direction(), q);
        if (bc2 < 0.f || bc1 + bc2 > det)
            return false;

        // 击中时光线行进距离
        rec.t = dot(e2, q);

        double inv_det = 1. / det;
        rec.t *= inv_det;
        bc1 *= inv_det;
        bc2 *= inv_det;

        // 避免数值误差造成交点在三角形内侧，反射再次击中三角形而被遮挡
        if (!interval.surrounds(rec.t))
            return false;

        // 重心坐标插值
        // 击中点
        rec.p = (1 - bc1 - bc2) * vertices[a_] + bc1 * vertices[b_] + bc2 * vertices[c_];
        // 法线
        Vec3 normal = (1 - bc1 - bc2) * normals[an_] + bc1 * normals[bn_] + bc2 * normals[cn_];
        rec.set_face_normal(r, normal);
        //  纹理坐标
        rec.u = (1 - bc1 - bc2) * texcoords[at_].first + bc1 * texcoords[bt_].first + bc2 * texcoords[ct_].first;
        rec.v = (1 - bc1 - bc2) * texcoords[at_].second + bc1 * texcoords[bt_].second + bc2 * texcoords[ct_].second;
        rec.material = material_;

        return true;
    }

    AABB get_bbox()
        const override
    {
        return bbox_;
    }
};

#endif // !TRIANGLE_H
