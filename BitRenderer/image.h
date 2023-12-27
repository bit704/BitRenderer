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

	void set_pixel(const int& row, const int& col, Color c, const int& samples_per_pixel);
	void set_pixel(const int& row, const int& col, const int& r, const int& g, const int& b);
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