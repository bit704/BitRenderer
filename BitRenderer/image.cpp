#include "image.h"
#include "logger.h"

// 以下头文件包含函数定义，不能包含在image.h中，否则当image.h被包含时会导致重定义错误LNK2005、LNK1169
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" 
#define __STDC_LIB_EXT1__ // 避免stb_image_write.h报C4996错误，未使用_s函数
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

/*
 * ImageWrite
 */

// static关键字只能用于类定义体内部的声明中，定义时不能标示为static
const std::string ImageWrite::kOutputPath_ = kOutputPath;

ImageWrite::ImageWrite(std::string imageName, int image_width, int image_height, int channel)
	: image_path_(kOutputPath_ + imageName), width_(image_width), height_(image_height), channel_(channel)
{
	// 使用stbi_image_free()释放，因此不用new
	image_data_ = (unsigned char*)malloc(width_ * height_ * channel_); // 初始化图片内存
	if(image_data_ != nullptr)
		memset(image_data_, 0, sizeof(unsigned char) * width_ * height_ * channel_);
}

// 伽马校正
double ImageWrite::linear_to_gamma(double linear_component)
{
	// 采用通用伽马值2.2，即放大暗部
	return pow(linear_component, 1 / 2.2);
}

void ImageWrite::set_pixel(const int& row, const int& col, const int& r, const int& g, const int& b)
{
	image_data_[(row * width_ + col) * channel_] = r;
	image_data_[(row * width_ + col) * channel_ + 1] = g;
	image_data_[(row * width_ + col) * channel_ + 2] = b;
	image_data_[(row * width_ + col) * channel_ + 3] = 255;
}

void ImageWrite::set_pixel(const int& row, const int& col, const Color3& c)
{
	// 伽马校正
	double r = linear_to_gamma(c.x());
	double g = linear_to_gamma(c.y());
	double b = linear_to_gamma(c.z());

	// 检测NaN
	if (r != r) r = 0.;
	if (g != g) g = 0.;
	if (b != b) b = 0.;

	// 将值限制在[0,1]，缩放到[0,256)，再向下取整
	r = std::clamp(r, 0., 1.) * 255.999;
	g = std::clamp(g, 0., 1.) * 255.999;
	b = std::clamp(b, 0., 1.) * 255.999;
	set_pixel(row, col, (int)r, (int)g, (int)b);
}

void ImageWrite::set_pixel(const int& row, const int& col, const Color3& c, const int& samples_per_pixel)
{
	// 一个像素采样几次就叠加了几个颜色，根据采样次数缩放回去
	double scale = 1. / samples_per_pixel;
	set_pixel(row, col, c * scale);
}

void ImageWrite::write()
{
	if (image_data_ == nullptr)
	{
		LOG("Write Fail");
		return;
	}
	stbi_write_png(image_path_.c_str(), width_, height_, channel_, image_data_, 0);
}

void ImageWrite::flush(Color3 c)
{
	int r = int(std::clamp(linear_to_gamma(c.x()), 0., 1.) * 255.999);
	int g = int(std::clamp(linear_to_gamma(c.y()), 0., 1.) * 255.999);
	int b = int(std::clamp(linear_to_gamma(c.z()), 0., 1.) * 255.999);

	for (int i = 0; i < height_; ++i)
		for (int j = 0; j < width_; ++j)
			set_pixel(i, j, r, g, b);
}

void ImageWrite::set_image_name(std::string image_name)
{
	image_path_ = kOutputPath_ + image_name;
}

ImageWrite::~ImageWrite()
{
	LOG("free the image for write");
	stbi_image_free(image_data_);
}

/*
 * ImageRead
 */

ImageRead::ImageRead(const std::string image_path)
{
	// req_comp为0代表不设置期望通道数，按实际来
	image_data_ = stbi_load(image_path.c_str(), &width_, &height_, &channel_, 0);
}

Color3 ImageRead::get_pixel(const int& row, const int& col) const
{
	unsigned char r = image_data_[(row * width_ + col) * channel_];
	unsigned char g = image_data_[(row * width_ + col) * channel_ + 1];
	unsigned char b = image_data_[(row * width_ + col) * channel_ + 2];

	auto scale = [](unsigned char x) -> double
	{
		return x / 255.;
	};
	return Color3(scale(r), scale(g), scale(b));
}

ImageRead::~ImageRead()
{
	LOG("free the image for read");
	stbi_image_free(image_data_);
}
