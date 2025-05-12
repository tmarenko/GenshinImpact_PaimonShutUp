#pragma once

#include <cstdint>
#include <memory>
#include <vector>

struct Color {
    uint8_t b, g, r;

    Color() : b(0), g(0), r(0) {}

    Color(uint8_t b, uint8_t g, uint8_t r) : b(b), g(g), r(r) {}

    bool operator>=(const Color &other) const {
        return b >= other.b && g >= other.g && r >= other.r;
    }

    bool operator<=(const Color &other) const {
        return b <= other.b && g <= other.g && r <= other.r;
    }
};

struct Rect {
    int x, y, width, height;

    Rect() : x(0), y(0), width(0), height(0) {}

    Rect(int x, int y, int width, int height)
            : x(x), y(y), width(width), height(height) {}
};

struct Size {
    int width, height;

    Size() : width(0), height(0) {}

    Size(int width, int height) : width(width), height(height) {}
};

class Image {
public:
    Image();

    Image(int height, int width, int channels = 3);

    ~Image() = default;

    void create(int height, int width, int channels = 3);

    bool empty() const;

    Size size() const;

    uint8_t *data();

    const uint8_t *data() const;

    int channels() const;

    int width() const;

    int height() const;

    static void cropAndThreshold(const Image &src, const Rect &rect, const Color &lower, const Color &upper, Image &dst);

private:
    int m_width;
    int m_height;
    int m_channels;
    std::vector<uint8_t> m_data;
};
