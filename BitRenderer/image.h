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

	Image(std::string image_name = "image.png", int image_width_ = 256, int image_height_ = 256, int channel_ = 3);
	~Image();

	void set_pixel(int row, int col, Color c, int samples_per_pixel = 1);
	void set_pixel(int row, int col, int r, int g, int b);
	void write();

private:

	static const std::string kOutputPath;
	unsigned char* image_data_;
	std::string image_path_;
	int image_width_;
	int image_height_;
	int channel_;
};

#endif // !IMAGE_H