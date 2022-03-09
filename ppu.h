#ifndef PPU_H
#define PPU_H

#include <stdint.h>

extern void write_ppu_addr(uint8_t hi_or_lo);
extern void write_ppu_data(uint8_t data);

/* tmp */
struct framebuffer;
extern void fill_bg_tile(struct framebuffer *fbuf, uint8_t *chr);

#endif /* _H */
