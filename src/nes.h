#ifndef NES_H
#define NES_H

#include <stdint.h>

#include "cpu.h"
#include "ppu.h"
#include "framebuffer.h"

namespace nes {

struct cartridge;
class FrameBuffer;

struct NES {
    struct CPU cpu;
    struct PPU ppu;
    uint64_t clock;

    int dma_wait;
    uint8_t dma_addr;
    uint8_t dma_page;
    uint8_t dma_data;

    FrameBuffer fbuf;
    FrameBuffer patt;
    struct cartridge *cart;
};

extern void power_up_nes(struct NES *nes);
extern void shut_down_nes(struct NES *nes);

extern void insert_cartridge(struct NES *nes, struct cartridge *cart);
extern void play_game(struct NES *nes);
extern void push_reset_button(struct NES *nes);

/* callback functions */
extern void update_frame(struct NES *nes);
extern void input_controller(struct NES *nes, uint8_t id, uint8_t input);

} // namespace

#endif // _H
