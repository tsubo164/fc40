#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

namespace nes {

class FrameBuffer;
struct PPU;
struct NES;

class Display {
public:
    struct NES *nes;

    const FrameBuffer *fb;
    FrameBuffer *pattern_table;

    void (*update_frame_func)(struct NES *nes);
    void (*input_controller_func)(struct NES *nes, uint8_t id, uint8_t input);

    // debug
    struct PPU *ppu;

    int Open() const;

private:
    void init_gl() const;
    void render() const;
};

} // namespace

#endif // _H
