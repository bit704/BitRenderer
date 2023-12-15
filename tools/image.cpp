#include<iostream>

#include"image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../tools/stb_image.h"
#define __STDC_LIB_EXT1__ // ±ÜÃâstb_image_write.h±¨C4996´íÎó
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../tools/stb_image_write.h"


const std::string Image::outputPath = "../output/";

Image::Image(std::string imageName, int imageWidth, int imageHeight, int channel)
	: imagePath(outputPath + imageName), imageWidth(imageWidth), imageHeight(imageHeight), channel(channel)
{
	imageData = (unsigned char*)malloc(this->imageWidth * this->imageHeight * this->channel); // ³õÊ¼»¯Í¼Æ¬ÄÚ´æ
}

void Image::setPixel(int row, int col, int r, int g, int b)
{
	imageData[(row * imageWidth + col) * channel] = r;
	imageData[(row * imageWidth + col) * channel + 1] = g;
	imageData[(row * imageWidth + col) * channel + 2] = b;
}

void Image::write()
{
	stbi_write_png(imagePath.c_str(), imageWidth, imageHeight, channel, imageData, 0);
}

Image::~Image()
{
	std::cout << "free the image"  << std::endl;
	std::cout << __FILE__ << std::endl;
	stbi_image_free(imageData);
}
