#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

// функция вычисления отступа по ширине
static int GetBMPStride(int w) {
    return 4 * ((w * 3 + 3) / 4);
}

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    BitmapInfoHeader(int width, int height)
        : biWidth(width), biHeight(height), biSizeImage(GetBMPStride(width) * height) {}
    
    uint32_t biSize = sizeof(BitmapInfoHeader);
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes = 1;
    uint16_t biBitCount = 24;
    uint32_t biCompression = 0;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter = 11811;
    int32_t biYPelsPerMeter = 11811;
    int32_t biClrUsed = 0;
    int32_t biClrImportant = 0x1000000;
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapFileHeader {
    BitmapFileHeader(int width, int height)
        : bfSize(sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + GetBMPStride(width) * height) {}
    
    char bfType[2] = {'B', 'M'};
    uint32_t bfSize;
    uint32_t bfReserved = 0; // uint16_t bfReserved1, bfReserved2 у майкрософт :)
    uint32_t bfOffBits = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);
}
PACKED_STRUCT_END

bool SaveBMP(const Path& file, const Image& image) {
    ofstream ofs(file, ios::binary);
    
    {
        BitmapFileHeader file_header(image.GetWidth(), image.GetHeight());
        BitmapInfoHeader info_header(image.GetWidth(), image.GetHeight());
        
        ofs.write(reinterpret_cast<const char*>(&file_header), sizeof(BitmapFileHeader));
        ofs.write(reinterpret_cast<const char*>(&info_header), sizeof(BitmapInfoHeader));
    }
    
    vector<char> buffer(GetBMPStride(image.GetWidth()));
    for (int y = image.GetHeight() - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);
        for (int x = 0; x < image.GetWidth(); ++x) {
            buffer[x * 3 + 0] = static_cast<char>(line[x].b);
            buffer[x * 3 + 1] = static_cast<char>(line[x].g);
            buffer[x * 3 + 2] = static_cast<char>(line[x].r);
        }
        
        ofs.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    }
    
    return ofs.good();
}

Image LoadBMP(const Path& file) {
    ifstream ifs(file, ios::binary);
    
    ifs.ignore(18); // ->biWidth
    
    int w, h;
    ifs.read(reinterpret_cast<char*>(&w), sizeof(w));
    ifs.read(reinterpret_cast<char*>(&h), sizeof(h));
    
    Image image(w, h, Color::Black());
    std::vector<char> buffer(GetBMPStride(w));
    
    ifs.ignore(28); // ->data
    
    for (int y = image.GetHeight() - 1; y >= 0; --y) {
        ifs.read(buffer.data(), buffer.size());
        
        Color* line = image.GetLine(y);
        for (int x = 0; x < image.GetWidth(); ++x) {
            line[x].b = static_cast<byte>(buffer[x * 3 + 0]);
            line[x].g = static_cast<byte>(buffer[x * 3 + 1]);
            line[x].r = static_cast<byte>(buffer[x * 3 + 2]);
        }
    }
    
    return image;
}

}  // namespace img_lib
