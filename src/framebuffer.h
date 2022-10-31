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
    FrameBuffer() : data_(width * height) {}
    ~FrameBuffer() {}

    int width = 1, height = 1;
    uint8_t *data;

    void Resize(int w, int h);
    void SetColor(int x, int y, Color col);
    void Clear();

private:
    std::vector<uint8_t> data_;
};

extern FrameBuffer *new_framebuffer(int width, int height);
extern void free_framebuffer(FrameBuffer *fb);

extern void set_color(FrameBuffer *fb, int x, int y, struct Color col);
extern void clear_color(FrameBuffer *fb);

} // namespace

#endif // _H
