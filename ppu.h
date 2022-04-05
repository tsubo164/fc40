#ifndef PPU_H
#define PPU_H

#include <stdint.h>

struct framebuffer;

struct PPU {
    const uint8_t *char_rom;
    size_t char_size;

    uint8_t ctrl;
    uint8_t mask;
    uint8_t stat;

    uint8_t addr_latch;
    uint16_t ppu_addr;
    uint8_t ppu_data_buf;
    uint8_t bg_palette_table[16];
    uint8_t name_table_0[0x03C0];

    uint8_t oam_addr;
    uint8_t oam[256]; /* 64 4 byte objects */

    int cycle;
    int scanline;
    struct framebuffer *fbuf;

    char nmi_generated;
};

/* interruptions */
extern void clear_nmi(struct PPU *ppu);
extern int is_nmi_generated(const struct PPU *ppu);
extern int is_frame_ready(const struct PPU *ppu);

/* clock */
extern void clock_ppu(struct PPU *ppu);

/* read registers */
extern void write_ppu_control(struct PPU *ppu, uint8_t data);
extern void write_ppu_mask(struct PPU *ppu, uint8_t data);
extern void write_oam_address(struct PPU *ppu, uint8_t addr);
extern void write_oam_data(struct PPU *ppu, uint8_t data);
extern void write_ppu_scroll(struct PPU *ppu, uint8_t data);
extern void write_ppu_address(struct PPU *ppu, uint8_t addr);
extern void write_ppu_data(struct PPU *ppu, uint8_t data);

/* write registers */
extern uint8_t read_ppu_status(struct PPU *ppu);
extern uint8_t read_oam_data(struct PPU *ppu);
extern uint8_t read_ppu_data(struct PPU *ppu);

#endif /* _H */
