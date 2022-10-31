#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

namespace nes {

class FrameBuffer;
struct PPU;
struct NES;

struct display {
    struct NES *nes;

    FrameBuffer *fb;
    FrameBuffer *pattern_table;

    void (*update_frame_func)(struct NES *nes);
    void (*input_controller_func)(struct NES *nes, uint8_t id, uint8_t input);

    /* debug */
    struct PPU *ppu;
};

extern int open_display(const struct display *disp);

} // namespace

#endif // _H
