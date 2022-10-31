#include <stdlib.h>
#include <algorithm>
#include "framebuffer.h"

namespace nes {

void FrameBuffer::Resize(int w, int h)
{
    width = w;
    height = h;
    data_.resize(width * height, 0);
}

void FrameBuffer::SetColor(int x, int y, Color col)
{
    const int index = y * width * 3 + x * 3;

    data_[index + 0] = col.r;
    data_[index + 1] = col.g;
    data_[index + 2] = col.b;
}

void FrameBuffer::Clear()
{
    std::fill(data_.begin(), data_.end(), 0);
}

FrameBuffer *new_framebuffer(int width, int height)
{
    FrameBuffer *fb = (FrameBuffer*) malloc(sizeof(FrameBuffer));
    uint8_t *data = (uint8_t*) calloc(width * height * 3, sizeof(uint8_t));

    fb->width = width;
    fb->height = height;
    fb->data = data;

    return fb;
}

void free_framebuffer(FrameBuffer *fb)
{
    if (!fb)
        return;
    free(fb->data);
    free(fb);
}

void set_color(FrameBuffer *fb, int x, int y, struct Color col)
{
    uint8_t *p = fb->data + y * fb->width * 3 + x * 3;

    p[0] = col.r;
    p[1] = col.g;
    p[2] = col.b;
}

void clear_color(FrameBuffer *fb)
{
    const struct Color black = {0};
    int x, y;

    for (y = 0; y < fb->height; y++)
        for (x=0; x < fb->width; x++)
            set_color(fb, x, y, black);
}

} // namespace
