#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <cstdint>
#include <vector>

namespace nes {

struct Color {
    uint8_t r = 0, g = 0, b = 0;
};

class FrameBuffer {
public:
    FrameBuffer();
    ~FrameBuffer() {}

    void Resize(int w, int h);
    void SetColor(int x, int y, Color col);

    int Width() const { return width_; }
    int Height() const { return height_; }
    const uint8_t *GetData() const { return &data_[0]; }

private:
    int width_, height_;
    std::vector<uint8_t> data_;
};

} // namespace

#endif // _H
