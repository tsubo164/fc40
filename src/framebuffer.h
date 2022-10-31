#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

namespace nes {

struct framebuffer {
    int width, height;
    uint8_t *data;
};

extern struct framebuffer *new_framebuffer(int width, int height);
extern void free_framebuffer(struct framebuffer *fb);

struct color {
    uint8_t r, g, b;
};

extern void set_color(struct framebuffer *fb, int x, int y, struct color col);
extern void clear_color(struct framebuffer *fb);

} // namespace

#endif // _H
