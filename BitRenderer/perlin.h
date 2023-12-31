/*
* 柏林噪声类
*/
#ifndef PERLIN_H
#define PERLIN_H

#include "common.h"
#include "point.h"

class Perlin
{

public:

    Perlin()
    {
        ranfloat_ = new double[point_count_];
        for (int i = 0; i < point_count_; ++i)
        {
            ranfloat_[i] = random_double();
        }

        perm_x_ = perlin_generate_perm();
        perm_y_ = perlin_generate_perm();
        perm_z_ = perlin_generate_perm();
    }

    ~Perlin()
    {
        delete[] ranfloat_;
        delete[] perm_x_;
        delete[] perm_y_;
        delete[] perm_z_;
    }

    double noise(const Point3& p) const
    {
        auto i = static_cast<int>(4 * p.x()) & 255;
        auto j = static_cast<int>(4 * p.y()) & 255;
        auto k = static_cast<int>(4 * p.z()) & 255;

        return ranfloat_[perm_x_[i] ^ perm_y_[j] ^ perm_z_[k]];
    }

private:

    static const int point_count_ = 256;

    double* ranfloat_;
    int* perm_x_;
    int* perm_y_;
    int* perm_z_;

    static int* perlin_generate_perm()
    {
        auto p = new int[point_count_];
        for (int i = 0; i < point_count_; i++)
            p[i] = i;
        permute(p, point_count_);
        return p;
    }

    static void permute(int* p, int n)
    {
        // 倒序遍历，每一数和前面一数随机交换位置
        for (int i = n - 1; i > 0; --i)
        {
            int target = random_int(0, i);
            int tmp = p[i];
            p[i] = p[target];
            p[target] = tmp;
        }
    }
};

#endif // !PERLIN_H