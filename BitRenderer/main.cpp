#include "../tools/image.h"

int main() 
{
    int imageWidth = 256;
    int imageHeight = 256;
    int channel = 3;

    std::unique_ptr<Image> image(new Image("image.png", imageWidth, imageHeight, channel));

    for (int i = 0; i < imageHeight; ++i) 
    {
        std::clog << "\rScanlines remaining: " << (imageHeight - i) << ' ' << std::flush;
        for (int j = 0; j < imageWidth; ++j) 
        {
            auto pixelColor = Color(double(i) / (imageWidth - 1), double(j) / (imageHeight - 1), 0).rescale();
            image->setPixel(i, j, pixelColor);
        }
    }
    std::clog << "\rDone.                 \n";

    image->write();
    return 0;
}