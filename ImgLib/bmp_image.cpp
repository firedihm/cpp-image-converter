#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <cstring>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

// функция вычисления отступа по ширине
static int GetBMPStride(int w) {
    return 4 * ((w * 3 + 3) / 4);
}

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    BitmapInfoHeader() = default;
    BitmapInfoHeader(int width, int height)
        : biSize(sizeof(BitmapInfoHeader)), biWidth(width), biHeight(height)
        , biPlanes(1), biBitCount(24), biCompression(0)
        , biSizeImage(GetBMPStride(width) * height)
        , biXPelsPerMeter(11811), biYPelsPerMeter(11811)
        , biClrUsed(0), biClrImportant(0x1000000) {
    }
    
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    int32_t biClrUsed;
    int32_t biClrImportant;
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapFileHeader {
    BitmapFileHeader() = default;
    BitmapFileHeader(int width, int height)
        : bfType{'B', 'M'}
        , bfSize(sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + GetBMPStride(width) * height)
        , bfReserved(0), bfOffBits(sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader)) {
    }
    
    char bfType[2];
    uint32_t bfSize;
    uint32_t bfReserved; // uint16_t bfReserved1, bfReserved2 у майкрософт :)
    uint32_t bfOffBits;
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

const char BMP_SIGNATURE[2] = {'B', 'M'};

Image LoadBMP(const Path& file) {
    ifstream ifs(file, ios::binary);
    
    BitmapFileHeader file_header;
    ifs.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    if (!ifs.good() || !std::strcmp(file_header.bfType, BMP_SIGNATURE)) {
        return {};
    }
    
    BitmapInfoHeader info_header;
    ifs.read(reinterpret_cast<char*>(&info_header), sizeof(info_header));
    if (!ifs.good()) {
        return {};
    }
    
    Image image(info_header.biWidth, info_header.biHeight, Color::Black());
    
    std::vector<char> buffer(GetBMPStride(image.GetWidth()));
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
