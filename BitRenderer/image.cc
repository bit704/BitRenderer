#include "image.h"

#include <iostream>
#include <string>
#include <new>
#include <algorithm>

// 以下头文件包含函数定义，不能包含在image.h中，否则当image.h被包含时会导致重定义错误LNK2005、LNK1169
#define STB_IMAGE_IMPLEMENTATION
#include "tool/stb_image.h"
#define __STDC_LIB_EXT1__ // 避免stb_image_write.h报C4996错误，未使用_s函数
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tool/stb_image_write.h"

#include "logger.h"

// static关键字只能用于类定义体内部的声明中，定义时不能标示为static
const std::string ImageWrite::kOutputPath_ = "../output/";

ImageWrite::ImageWrite(std::string imageName, int image_width, int image_height, int channel)
	: image_path_(kOutputPath_ + imageName), width_(image_width), height_(image_height), channel_(channel)
{
	// 使用stbi_image_free()释放，因此不用new
	image_data_ = (unsigned char*)malloc(this->width_ * this->height_ * this->channel_); // 初始化图片内存
}

// 设置像素
void ImageWrite::set_pixel(const int& row, const int& col, const int& r, const int& g, const int& b)
{
	image_data_[(row * width_ + col) * channel_] = r;
	image_data_[(row * width_ + col) * channel_ + 1] = g;
	image_data_[(row * width_ + col) * channel_ + 2] = b;
}

// 伽马校正
inline double linear_to_gamma(double linear_component)
{
	// 采用通用伽马值2.2
	return pow(linear_component, 1 / 2.2);
}

void ImageWrite::set_pixel(const int& row, const int& col, Color c, const int& samples_per_pixel)
{
	// 一个像素采样几次就叠加了几个颜色，根据采样次数缩放回去
	double scale = 1. / samples_per_pixel;
	c *= scale;

	// 伽马校正
	double r = linear_to_gamma(c.x());
	double g = linear_to_gamma(c.y());
	double b = linear_to_gamma(c.z());
	
	// 将值限制在[0,1]，缩放到[0,256)，再向下取整
	r = std::clamp(r, 0., 1.) * 255.999;
	g = std::clamp(g, 0., 1.) * 255.999;
	b = std::clamp(b, 0., 1.) * 255.999;
	set_pixel(row, col, (int)r, (int)g, (int)b);
}

// 写入指定图片
void ImageWrite::write()
{
	stbi_write_png(image_path_.c_str(), width_, height_, channel_, image_data_, 0);
}

ImageWrite::~ImageWrite()
{
	LOG("free the image");
	stbi_image_free(image_data_);
}

const std::string ImageRead::kInputPath_ = "../texture/";

ImageRead::ImageRead(std::string image_name) : channel_(3)
{
	image_data_ = stbi_load(image_name.c_str(), &width_, &height_, &channel_, channel_);
}

Color ImageRead::get_pixel(const int& row, const int& col) const
{
	unsigned char r = image_data_[(row * width_ + col) * channel_];
	unsigned char g = image_data_[(row * width_ + col) * channel_ + 1];
	unsigned char b = image_data_[(row * width_ + col) * channel_ + 2];

	auto scale = [](unsigned char x) -> double
	{
		return x / 255.;
	};
	return Color(scale(r), scale(g), scale(b));
}
