/*
* 纹理类
*/
#ifndef TEXTURE_H
#define TEXTURE_H

#include <algorithm>

#include "color.h"
#include "point.h"
#include "perlin.h"

class Texture
{
public:

    virtual ~Texture() = default;
    virtual Color value(double u, double v, const Point3& p) const = 0;
};

// 坐标无关恒定颜色纹理
class SolidColor : public Texture
{
public:

    SolidColor(Color c) : color_value_(c) {}
    SolidColor(double r, double g, double b) : SolidColor(Color(r, g, b)) {}

    Color value(double u, double v, const Point3& p) const override
    {
        return color_value_;
    }

private:

    Color color_value_;
};

// 棋盘格3D纹理，仅与空间坐标p相关
class CheckerTexture : public Texture
{
public:

    CheckerTexture(double _scale, std::shared_ptr<Texture> _even, std::shared_ptr<Texture> _odd)
        : inv_scale(1.0 / _scale), even_(_even), odd_(_odd) {}

    CheckerTexture(double _scale, Color c1, Color c2)
        : inv_scale(1.0 / _scale),
        even_(std::make_shared<SolidColor>(c1)),
        odd_(std::make_shared<SolidColor>(c2)) {}

    Color value(double u, double v, const Point3& p) const override
    {
        auto x_int = static_cast<int>(std::floor(inv_scale * p.x()));
        auto y_int = static_cast<int>(std::floor(inv_scale * p.y()));
        auto z_int = static_cast<int>(std::floor(inv_scale * p.z()));
        bool is_even = (x_int + y_int + z_int) % 2 == 0;

        return is_even ? even_->value(u, v, p) : odd_->value(u, v, p);
    }

private:

    double inv_scale;
    std::shared_ptr<Texture> even_;
    std::shared_ptr<Texture> odd_;
};

class ImageTexture : public Texture
{
public:

    ImageTexture(std::string filename) : image_read_(filename) {}

    Color value(double u, double v, const Point3& p) const override
    {
        // 纯红说明纹理没有读取成功
        if (image_read_.get_height() <= 0) 
            return Color(1., 0., 0.);

        u = std::clamp(u, 0., 1.);
        //v = std::clamp(v, 0., 1.); // 不翻转v
        v = 1.0 - std::clamp(v, 0., 1.); // 翻转v

        int i = static_cast<int>(v * image_read_.get_height());
        int j = static_cast<int>(u * image_read_.get_width());
        return image_read_.get_pixel(i, j);
    }

private:

    ImageRead image_read_;
};

class NoiseTexture : public Texture
{
public:

    NoiseTexture() {}

    // 频率
    NoiseTexture(double sc) : scale_(sc) {}

    Color value(double u, double v, const Point3& p) const override
    {
        return Color(1, 1, 1) * 0.5 * (1.0 + noise_.noise(scale_ * p));
    }

private:

    Perlin noise_;
    double scale_;
};

#endif // !TEXTURE_H

