#include "image.h"

// 以下头文件包含函数定义，不能包含在image.h中，否则当image.h被包含时会导致重定义错误LNK2005、LNK1169
#define STB_IMAGE_IMPLEMENTATION
#include "../tools/stb_image.h"
#define __STDC_LIB_EXT1__ // 避免stb_image_write.h报C4996错误，未使用_s函数
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../tools/stb_image_write.h"

// static 关键字只能用于类定义体内部的声明中，定义时不能标示为 static
const std::string Image::outputPath = "../output/";

Image::Image(std::string imageName, int imageWidth, int imageHeight, int channel)
	: imagePath(outputPath + imageName), imageWidth(imageWidth), imageHeight(imageHeight), channel(channel)
{
	// 使用stbi_image_free()释放，不能用new
	imageData = (unsigned char*)malloc(this->imageWidth * this->imageHeight * this->channel); // 初始化图片内存
}

// 设置像素
void Image::setPixel(int row, int col, int r, int g, int b)
{
	imageData[(row * imageWidth + col) * channel] = r;
	imageData[(row * imageWidth + col) * channel + 1] = g;
	imageData[(row * imageWidth + col) * channel + 2] = b;
}

void Image::setPixel(int row, int col, color c)
{
	setPixel(row, col, (int)c.e[0], (int)c.e[1], (int)c.e[2]);
}

// 写入指定图片
void Image::write()
{
	stbi_write_png(imagePath.c_str(), imageWidth, imageHeight, channel, imageData, 0);
}

Image::~Image()
{
	LOG("free the image");
	stbi_image_free(imageData);
}
