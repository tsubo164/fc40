#ifndef PPU_H
#define PPU_H

#include <stdint.h>

struct framebuffer;

enum ppu_register {
    PPUCTRL,
    PPUMASK,
    PPUSTATUS,
    OAMADDR,
    OAMDATA,
    PPUSCROLL,
    PPUADDR,
    PPUDATA,
    OAMDMA
};

struct PPU {
    const uint8_t *char_rom;
    size_t char_size;
    uint8_t stat;

    uint16_t ppu_addr;
    uint8_t ppu_data_buf;
    uint8_t bg_palette_table[16];
    uint8_t name_table_0[0x03C0];

    int cycle;
    int scanline;
    struct framebuffer *fbuf;
};

extern int is_frame_ready(const struct PPU *ppu);
extern void clock_ppu(struct PPU *ppu);

extern void write_ppu_addr(struct PPU *ppu, uint8_t hi_or_lo);
extern void write_ppu_data(struct PPU *ppu, uint8_t data);

extern uint8_t ppu_read_register(struct PPU *ppu, int reg);

extern uint8_t read_ppu_status(struct PPU *ppu);

#endif /* _H */
