#ifndef PPU_H
#define PPU_H

#include <stdint.h>

struct PPU {
    const uint8_t *char_rom;
    size_t char_size;
};

extern void write_ppu_addr(uint8_t hi_or_lo);
extern void write_ppu_data(uint8_t data);

/* tmp */
struct framebuffer;
extern void fill_bg_tile(struct framebuffer *fbuf, uint8_t *chr);

extern void set_pixel_color(struct framebuffer *fbuf, const uint8_t *chr, int x, int y);

#endif /* _H */
