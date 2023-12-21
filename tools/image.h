/*
* 图片类
* 用于写入png格式图片
*/
#ifndef IMAGE_H
#define IMAGE_H

#include "color.h"

class Image 
{
public:

	std::string image_path;
	int image_width;
	int image_height;
	int channel;

	Image(std::string image_name = "image.png", int image_width = 256, int image_height = 256, int channel = 3);
	~Image();

	void set_pixel(int row, int col, Color c);
	void set_pixel(int row, int col, int r, int g, int b);
	void write();

private:

	static const std::string output_path;
	unsigned char* image_data;
};

#endif // !IMAGE_H