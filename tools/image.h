/*
* 图片类
* 用于写入png格式图片
*/
#ifndef IMAGE_H
#define IMAGE_H

#include <iostream>
#include <string>
#include <new>

#include "logger.h"

class Image 
{
public:

	std::string imagePath;
	int imageWidth;
	int imageHeight;
	int channel;

	Image(std::string imageName = "image.png", int imageWidth = 256, int imageHeight = 256, int channel = 3);
	~Image();

	void setPixel(int row, int col, int r, int g, int b);
	void write();

private:

	static const std::string outputPath;
	unsigned char* imageData;
};

#endif // !IMAGE_H