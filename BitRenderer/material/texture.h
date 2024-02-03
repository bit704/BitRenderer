/*
 * 纹理类
 */
#ifndef TEXTURE_H
#define TEXTURE_H

#include "vec.h"
#include "perlin.h"

class Texture
{
public:
    virtual ~Texture() = default;
    virtual Color3 value(double u, double v, const Point3& p) const = 0;
};

// 坐标无关恒定颜色纹理
class SolidColor : public Texture
{
private:
    Color3 color_value_;

public:
    SolidColor(Color3 c) : color_value_(c) {}
    SolidColor(double r, double g, double b) : SolidColor(Color3(r, g, b)) {}

public:
    Color3 value(double u, double v, const Point3& p) 
        const override
    {
        return color_value_;
    }
};

// 棋盘格3D纹理，仅与空间坐标p相关
class CheckerTexture : public Texture
{
private:
    double inv_scale_;
    shared_ptr<Texture> even_, odd_;

public:
    CheckerTexture(double scale, shared_ptr<Texture> even, shared_ptr<Texture> odd)
        : inv_scale_(1.0 / scale), even_(even), odd_(odd) {}

    CheckerTexture(double scale, Color3 c1, Color3 c2)
        : inv_scale_(1.0 / scale),
        even_(std::make_shared<SolidColor>(c1)),
        odd_(std::make_shared<SolidColor>(c2)) {}

    CheckerTexture(const CheckerTexture&) = delete;
    CheckerTexture& operator=(const CheckerTexture&) = delete;

    CheckerTexture(CheckerTexture&&) = delete;
    CheckerTexture& operator=(CheckerTexture&&) = delete;

public:
    Color3 value(double u, double v, const Point3& p)
        const override
    {
        auto x_int = static_cast<int>(std::floor(inv_scale_ * p.x()));
        auto y_int = static_cast<int>(std::floor(inv_scale_ * p.y()));
        auto z_int = static_cast<int>(std::floor(inv_scale_ * p.z()));
        bool is_even = (x_int + y_int + z_int) % 2 == 0;

        return is_even ? even_->value(u, v, p) : odd_->value(u, v, p);
    }
};

// 图片纹理
class ImageTexture : public Texture
{
private:
    ImageRead image_read_;

public:
    ImageTexture(std::string filename) : image_read_(filename) {}

public:
    Color3 value(double u, double v, const Point3& p)
        const override
    {
        // 纯红说明纹理没有读取成功
        if (image_read_.get_image_height() <= 0) 
            return Color3(1., 0., 0.);

        u = std::clamp(u, 0., 1.);
        //v = std::clamp(v, 0., 1.); // 不翻转v
        // uv以左下角为原点而图片以左上角为原点时
        v = 1.0 - std::clamp(v, 0., 1.); // 翻转v

        int i = static_cast<int>(v * image_read_.get_image_height());
        int j = static_cast<int>(u * image_read_.get_image_width());
        return image_read_.get_pixel(i, j);
    }
};

// 噪声纹理
class NoiseTexture : public Texture
{
private:
    Perlin noise_;
    double scale_ = 1.;

public:
    // 频率
    NoiseTexture(double sc) : scale_(sc) {}

public:
    Color3 value(double u, double v, const Point3& p) 
        const override
    {
        auto s = scale_ * p;
        return Color3(1, 1, 1) * .5 * (1 + sin(s.z() + 10 * noise_.turb(s)));
    }
};

#endif // !TEXTURE_H

