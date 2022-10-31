#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <cstdint>
#include <vector>

namespace nes {

class FrameBuffer {
public:
    FrameBuffer() {}
    ~FrameBuffer() {}

    int width, height;
    uint8_t *data;

private:
    std::vector<uint8_t> data_;
};

extern FrameBuffer *new_framebuffer(int width, int height);
extern void free_framebuffer(FrameBuffer *fb);

struct color {
    uint8_t r, g, b;
};

extern void set_color(FrameBuffer *fb, int x, int y, struct color col);
extern void clear_color(FrameBuffer *fb);

} // namespace

#endif // _H
