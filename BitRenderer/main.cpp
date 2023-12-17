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
            auto r = double(j) / (imageWidth - 1);
            auto g = double(i) / (imageHeight - 1);
            auto b = 0;
            int ir = static_cast<int>(255.999 * r);
            int ig = static_cast<int>(255.999 * g);
            int ib = static_cast<int>(255.999 * b);

            image->setPixel(i, j, ir, ig, ib);
        }
    }
    std::clog << "\rDone.                 \n";

    image->write();
    return 0;
}