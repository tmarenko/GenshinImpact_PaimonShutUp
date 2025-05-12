#include "image.h"
#include <algorithm>
#include <cstring>

Image::Image() : m_width(0), m_height(0), m_channels(0) {}

Image::Image(int height, int width, int channels)
        : m_width(width), m_height(height), m_channels(channels) {
    m_data.resize(width * height * channels);
}

void Image::create(int height, int width, int channels) {
    m_width = width;
    m_height = height;
    m_channels = channels;
    m_data.resize(width * height * channels);
}

bool Image::empty() const {
    return m_data.empty();
}

Size Image::size() const {
    return Size(m_width, m_height);
}

uint8_t *Image::data() {
    return m_data.data();
}

const uint8_t *Image::data() const {
    return m_data.data();
}

int Image::channels() const {
    return m_channels;
}

int Image::width() const {
    return m_width;
}

int Image::height() const {
    return m_height;
}

void Image::cropAndThreshold(const Image &src, const Rect &rect,
                             const Color &lower, const Color &upper, Image &dst) {
    if (dst.width() != rect.width || dst.height() != rect.height) {
        dst.create(rect.height, rect.width, 1);
    }

    uint8_t *dstData = dst.data();
    const uint8_t *srcData = src.data();
    const int srcChannels = src.channels();
    const int srcWidth = src.width();
    const int dstWidth = rect.width;
    const int srcRowStride = srcWidth * srcChannels;
    const int srcOffset = (rect.y * srcWidth + rect.x) * srcChannels;

    for (int y = 0; y < rect.height; ++y) {
        const uint8_t *srcRow = &srcData[srcOffset + y * srcRowStride];
        uint8_t *dstRow = &dstData[y * dstWidth];

        for (int x = 0; x < rect.width; ++x) {
            const uint8_t *pixel = &srcRow[x * srcChannels];
            Color color(pixel[2], pixel[1], pixel[0]); // Convert BGR to RGB format
            dstRow[x] = (color >= lower && color <= upper) ? 255 : 0;
        }
    }
}
