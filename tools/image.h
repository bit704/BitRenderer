/*
* 图片类
* 用于写入png格式图片
*/

#include<string>

class Image 
{
private:

	static const std::string outputPath;

	unsigned char* imageData;

public:

	std::string imagePath;

	int imageWidth;
	
	int imageHeight;

	int channel;


	Image(std::string imageName = "image.png", int imageWidth = 256, int imageHeight = 256, int channel = 3);

	void setPixel(int row, int col, int r, int g, int b);

	void write();

	~Image();

};