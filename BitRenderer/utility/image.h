/*
 * 图片写入类
 * 用于写入png格式图片
 * 
 * 图片读入类
 * 用于读入纹理图片
 * 
 * 除读写外，像素值在运算时值域为[0,1]
 * 写入像素值时进行伽马校正
 */
#ifndef IMAGE_H
#define IMAGE_H

#include "vec.h"
#include "common.h"

class ImageWrite 
{
private:
	static const std::string kOutputPath_;
	unsigned char* image_data_;
	std::string image_path_;
	int width_, height_;
	int channel_ = 4;

public:
	ImageWrite(std::string image_name, int width_, int height_, int channel_);

	~ImageWrite();

	ImageWrite(const ImageWrite&) = delete;
	ImageWrite& operator=(const ImageWrite&) = delete;

	ImageWrite(ImageWrite&&) = delete;
	ImageWrite& operator=(ImageWrite&&) = delete;

public:
	// 直接设置图像像素值（值域为[0,255]的整数）
	void set_pixel(const int& row, const int& col, const int& r, const int& g, const int& b);
	// 设置图像像素为Color3 c（值域为[0,1]的浮点数）,先伽马矫正再缩放
	void set_pixel(const int& row, const int& col, const Color3& c);
	// 根据samples_per_pixel先缩放，再设置
	void set_pixel(const int& row, const int& col, const Color3& c, const int& samples_per_pixel);
	// 写入指定图片
	void write();
	void flush(Color3 c);
	void set_image_name(std::string image_name);

	unsigned char** get_image_data_p2p()
	{
		if (image_data_ == nullptr)
			return nullptr;
		return &image_data_;
	}

private:
	double linear_to_gamma(double linear_component);
};

class ImageRead
{
private:
	static const std::string kInputPath_;
	unsigned char* image_data_;
	std::string image_path_;
	int width_, height_;
	int channel_;

public:
	ImageRead(std::string image_name);

	~ImageRead();

	ImageRead(const ImageRead&) = delete;
	ImageRead& operator=(const ImageRead&) = delete;

	ImageRead(ImageRead&&) = delete;
	ImageRead& operator=(ImageRead&&) = delete;


public:
	Color3 get_pixel(const int& row, const int& col) const;

	int get_image_width() const
	{
		return width_;
	}

	int get_image_height() const
	{
		return height_;
	}
};

#endif // !IMAGE_H