#ifndef PPU_H
#define PPU_H

#include <stdint.h>

struct framebuffer;

struct ppu_status {
    char unused[5];
    char overflow;
    char zerohit;
    char vblank;
};

struct PPU {
    const uint8_t *char_rom;
    size_t char_size;
    struct ppu_status stat;

    int cycle;
    int scanline;
    struct framebuffer *fbuf;
};

extern int is_frame_ready(const struct PPU *ppu);
extern void clock_ppu(struct PPU *ppu);

extern void write_ppu_addr(uint8_t hi_or_lo);
extern void write_ppu_data(uint8_t data);

#endif /* _H */
