#include "image.h"

#include <iostream>
#include <string>
#include <new>

// 以下头文件包含函数定义，不能包含在image.h中，否则当image.h被包含时会导致重定义错误LNK2005、LNK1169
#define STB_IMAGE_IMPLEMENTATION
#include "tool/stb_image.h"
#define __STDC_LIB_EXT1__ // 避免stb_image_write.h报C4996错误，未使用_s函数
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tool/stb_image_write.h"

#include "logger.h"

// static 关键字只能用于类定义体内部的声明中，定义时不能标示为 static
const std::string Image::output_path_ = "../output/";

Image::Image(std::string imageName, int image_width, int image_height, int channel)
	: image_path_(output_path_ + imageName), image_width_(image_width), image_height_(image_height), channel_(channel)
{
	// 使用stbi_image_free()释放，不能用new
	image_data_ = (unsigned char*)malloc(this->image_width_ * this->image_height_ * this->channel_); // 初始化图片内存
}

// 设置像素
void Image::set_pixel(int row, int col, int r, int g, int b)
{
	image_data_[(row * image_width_ + col) * channel_] = r;
	image_data_[(row * image_width_ + col) * channel_ + 1] = g;
	image_data_[(row * image_width_ + col) * channel_ + 2] = b;
}

void Image::set_pixel(int row, int col, Color c)
{
	c.rescale();
	set_pixel(row, col, (int)c.e_[0], (int)c.e_[1], (int)c.e_[2]);
}

// 写入指定图片
void Image::write()
{
	stbi_write_png(image_path_.c_str(), image_width_, image_height_, channel_, image_data_, 0);
}

Image::~Image()
{
	LOG("free the image");
	stbi_image_free(image_data_);
}
