#ifndef PPU_H
#define PPU_H

#include <stdint.h>

struct framebuffer;
struct cartridge;

struct pattern_row {
    uint8_t id;
    uint8_t lo, hi;
    uint8_t palette_lo;
    uint8_t palette_hi;
};

struct object_attribute {
    uint8_t id;
    uint8_t attr;
    uint8_t x, y;
};

struct PPU {
    struct cartridge *cart;

    /* registers */
    uint8_t ctrl;
    uint8_t mask;
    uint8_t stat;

    /* vram and scroll */
    uint8_t write_toggle;
    uint16_t vram_addr;
    uint16_t temp_addr;
    uint8_t fine_x;

    /* vram */
    uint8_t ppu_data_buf;
    uint8_t palette_ram[32];
    uint8_t name_table_0[1024];

    /* bg tile cache */
    struct pattern_row tile_queue[3];

    /* fg sprite */
    uint8_t oam_addr;
    uint8_t oam[256];
    struct object_attribute secondary_oam[8];
    /* 8 latches and 8 counters */
    struct object_attribute rendering_oam[8];
    struct pattern_row rendering_sprite[8];
    int sprite_count;

    int cycle;
    int scanline;
    uint64_t frame;
    struct framebuffer *fbuf;

    uint8_t nmi_generated;
};

/* interruptions */
extern void clear_nmi(struct PPU *ppu);
extern int is_nmi_generated(const struct PPU *ppu);
extern int is_frame_ready(const struct PPU *ppu);

/* clock */
extern void clock_ppu(struct PPU *ppu);

/* write registers */
extern void write_ppu_control(struct PPU *ppu, uint8_t data);
extern void write_ppu_mask(struct PPU *ppu, uint8_t data);
extern void write_oam_address(struct PPU *ppu, uint8_t addr);
extern void write_oam_data(struct PPU *ppu, uint8_t data);
extern void write_ppu_scroll(struct PPU *ppu, uint8_t data);
extern void write_ppu_address(struct PPU *ppu, uint8_t addr);
extern void write_ppu_data(struct PPU *ppu, uint8_t data);

/* read registers */
extern uint8_t read_ppu_status(struct PPU *ppu);
extern uint8_t read_oam_data(const struct PPU *ppu);
extern uint8_t read_ppu_data(const struct PPU *ppu);

/* peek registers */
extern uint8_t peek_ppu_status(const struct PPU *ppu);

/* sprites */
extern void write_dma_sprite(struct PPU *ppu, uint8_t addr, uint8_t data);

/* debug */
extern struct object_attribute read_oam(const struct PPU *ppu, int index);

#endif /* _H */
