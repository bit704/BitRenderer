#include "image.h"

#include <iostream>
#include <string>
#include <new>

// 以下头文件包含函数定义，不能包含在image.h中，否则当image.h被包含时会导致重定义错误LNK2005、LNK1169
#define STB_IMAGE_IMPLEMENTATION
#include "../tools/stb_image.h"
#define __STDC_LIB_EXT1__ // 避免stb_image_write.h报C4996错误，未使用_s函数
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../tools/stb_image_write.h"

#include "logger.h"

// static 关键字只能用于类定义体内部的声明中，定义时不能标示为 static
const std::string Image::output_path = "../output/";

Image::Image(std::string imageName, int image_width, int image_height, int channel)
	: image_path(output_path + imageName), image_width(image_width), image_height(image_height), channel(channel)
{
	// 使用stbi_image_free()释放，不能用new
	image_data = (unsigned char*)malloc(this->image_width * this->image_height * this->channel); // 初始化图片内存
}

// 设置像素
void Image::set_pixel(int row, int col, int r, int g, int b)
{
	image_data[(row * image_width + col) * channel] = r;
	image_data[(row * image_width + col) * channel + 1] = g;
	image_data[(row * image_width + col) * channel + 2] = b;
}

void Image::set_pixel(int row, int col, Color c)
{
	c.rescale();
	set_pixel(row, col, (int)c.e[0], (int)c.e[1], (int)c.e[2]);
}

// 写入指定图片
void Image::write()
{
	stbi_write_png(image_path.c_str(), image_width, image_height, channel, image_data, 0);
}

Image::~Image()
{
	LOG("free the image");
	stbi_image_free(image_data);
}
