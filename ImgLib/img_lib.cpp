#include "img_lib.h"

namespace img_lib {

Color* Image::GetLine(int y) {
    assert(y >= 0 && y < height_);
    return pixels_.data() + step_ * y;
}

const Color* Image::GetLine(int y) const {
    return const_cast<Image*>(this)->GetLine(y);
}

Color& Image::GetPixel(int x, int y) {
    assert(x < GetWidth() && y < GetHeight() && x >= 0 && y >= 0);
    return GetLine(y)[x];
}

Color Image::GetPixel(int x, int y) const {
    return const_cast<Image*>(this)->GetPixel(x, y);
}

}  // namespace img_lib
