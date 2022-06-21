#ifndef NES_H
#define NES_H

#include <stdint.h>

#include "cpu.h"
#include "ppu.h"

struct cartridge;
struct framebuffer;

struct NES {
    struct CPU cpu;
    struct PPU ppu;
    uint64_t clock;

    int dma_wait;
    uint8_t dma_addr;
    uint8_t dma_page;
    uint8_t dma_data;

    struct framebuffer *fbuf;
    struct framebuffer *patt;
    struct cartridge *cart;
};

extern void power_up_nes(struct NES *nes);
extern void shut_down_nes(struct NES *nes);

extern void insert_cartridge(struct NES *nes, struct cartridge *cart);

/* callback functions */
extern void update_frame(struct NES *nes);
extern void input_controller(struct NES *nes, uint8_t id, uint8_t input);

#endif /* _H */
