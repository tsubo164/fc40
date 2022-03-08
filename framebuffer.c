#include <stdlib.h>
#include "framebuffer.h"

struct framebuffer *new_framebuffer(int width, int height)
{
    struct framebuffer *fbuf = malloc(sizeof(struct framebuffer));
    uint8_t *data = calloc(width * height * 3, sizeof(uint8_t));

    fbuf->width = width;
    fbuf->height = height;
    fbuf->data = data;

    return fbuf;
}

void free_framebuffer(struct framebuffer *fbuf)
{
    if (!fbuf)
        return;
    free(fbuf->data);
    free(fbuf);
}

void set_color(struct framebuffer *fbuf, int x, int y, const uint8_t *rgb)
{
    uint8_t *p = fbuf->data + y * fbuf->width * 3 + x * 3;
    p[0] = rgb[0];
    p[1] = rgb[1];
    p[2] = rgb[2];
}
