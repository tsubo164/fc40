#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

struct framebuffer {
    int width, height;
    uint8_t *data;
};

extern struct framebuffer *new_framebuffer(int width, int height);
extern void free_framebuffer(struct framebuffer *fb);

extern void set_color(struct framebuffer *fb, int x, int y, const uint8_t *rgb);
extern void clear_color(struct framebuffer *fb);

#endif /* _H */
