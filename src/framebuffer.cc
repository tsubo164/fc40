#include <stdlib.h>
#include "framebuffer.h"

namespace nes {

struct framebuffer *new_framebuffer(int width, int height)
{
    struct framebuffer *fb = (struct framebuffer*) malloc(sizeof(struct framebuffer));
    uint8_t *data = (uint8_t*) calloc(width * height * 3, sizeof(uint8_t));

    fb->width = width;
    fb->height = height;
    fb->data = data;

    return fb;
}

void free_framebuffer(struct framebuffer *fb)
{
    if (!fb)
        return;
    free(fb->data);
    free(fb);
}

void set_color(struct framebuffer *fb, int x, int y, struct color col)
{
    uint8_t *p = fb->data + y * fb->width * 3 + x * 3;

    p[0] = col.r;
    p[1] = col.g;
    p[2] = col.b;
}

void clear_color(struct framebuffer *fb)
{
    const struct color black = {0};
    int x, y;

    for (y = 0; y < fb->height; y++)
        for (x=0; x < fb->width; x++)
            set_color(fb, x, y, black);
}

} // namespace
