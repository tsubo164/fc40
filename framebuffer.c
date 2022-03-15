#include <stdlib.h>
#include "framebuffer.h"

struct framebuffer *new_framebuffer(int width, int height)
{
    struct framebuffer *fb = malloc(sizeof(struct framebuffer));
    uint8_t *data = calloc(width * height * 3, sizeof(uint8_t));

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

void set_color(struct framebuffer *fb, int x, int y, const uint8_t *rgb)
{
    uint8_t *p = fb->data + y * fb->width * 3 + x * 3;
    p[0] = rgb[0];
    p[1] = rgb[1];
    p[2] = rgb[2];
}

void clear_color(struct framebuffer *fb)
{
    const uint8_t black[3] = {0};
    int x, y;

    for (y = 0; y < fb->height; y++)
        for (x=0; x < fb->width; x++)
            set_color(fb, x, y, black);
}
