#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <vector>

namespace img_lib {

struct Size {
    int width;
    int height;
};

struct Color {
    static Color Black() {
        return {std::byte{0}, std::byte{0}, std::byte{0}, std::byte{255}};
    }
    
    std::byte r, g, b, a;
};

class Image {
public:
    Image() = default;
    Image(int w, int h, Color fill) : width_(w), height_(h), step_(w), pixels_(step_ * height_, fill) {}
    
    // будем считать изображение корректным, если
    // его площадь положительна
    explicit operator bool() const { return GetWidth() > 0 && GetHeight() > 0; }
    bool operator!() const { return !operator bool(); }
    
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    int GetStep() const { return step_; }
    
    // геттер для заданной строки изображения
    Color* GetLine(int y);
    const Color* GetLine(int y) const;
    
    // геттеры для отдельного пикселя изображения
    Color& GetPixel(int x, int y);
    Color GetPixel(int x, int y) const;
    
private:
    // шаг задаёт смещение соседних строк изображения
    // он обычно совпадает с шириной, но может быть больше неё
    int width_ = 0;
    int height_ = 0;
    int step_;
    
    std::vector<Color> pixels_;
};

}  // namespace img_lib
