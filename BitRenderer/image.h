/*
* 图片写入类
* 用于写入png格式图片
* 图片读入类
* 用于读入纹理图片
* 
* 除读写外，像素值在运算时值域为[0,1]
*/
#ifndef IMAGE_H
#define IMAGE_H

#include "color.h"

class ImageWrite 
{
public:

	ImageWrite(std::string image_name, int width_, int height_, int channel_);
	~ImageWrite();

	void set_pixel(const int& row, const int& col, Color c, const int& samples_per_pixel);
	void set_pixel(const int& row, const int& col, const int& r, const int& g, const int& b);
	void write();

private:

	static const std::string kOutputPath_;
	unsigned char* image_data_;
	std::string image_path_;
	int width_;
	int height_;
	int channel_;
};

class ImageRead
{
public:

	ImageRead(std::string image_name);

	Color get_pixel(const int& row, const int& col) const;

	int get_width() const
	{
		return width_;
	}

	int get_height() const
	{
		return height_;
	}

private:

	static const std::string kInputPath_;
	unsigned char* image_data_;
	std::string image_path_;
	int width_;
	int height_;
	int channel_;
};

#endif // !IMAGE_H