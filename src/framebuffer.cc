#include "framebuffer.h"

namespace nes {

FrameBuffer::FrameBuffer()
{
    Resize(1, 1);
}

void FrameBuffer::Resize(int w, int h)
{
    width_ = w;
    height_ = h;
    data_.resize(Width() * Height() * 3, 0);
}

void FrameBuffer::SetColor(int x, int y, Color col)
{
    const int index = y * Width() * 3 + x * 3;

    data_[index + 0] = col.r;
    data_[index + 1] = col.g;
    data_[index + 2] = col.b;
}

} // namespace
