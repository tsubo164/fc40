#ifndef PPU_H
#define PPU_H

#include <stdint.h>

struct framebuffer;

struct PPU {
    const uint8_t *char_rom;
    size_t char_size;

    int x, y;
    struct framebuffer *fbuf;
};

extern void clock_ppu(struct PPU *ppu);

extern void write_ppu_addr(uint8_t hi_or_lo);
extern void write_ppu_data(uint8_t data);

#endif /* _H */
