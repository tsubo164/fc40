#include <stdlib.h>
#include "ppu.h"
#include "framebuffer.h"

enum ppu_status {
    STAT_UNUSED          = 0x1F,
    STAT_SPRITE_OVERFLOW = 1 << 5,
    STAT_SPRITE_ZERO_HIT = 1 << 6,
    STAT_VERTICAL_BLANK  = 1 << 7,
};

enum ppu_control {
    CTRL_NAMETABLE_X     = 1 << 0,
    CTRL_NAMETABLE_Y     = 1 << 1,
    CTRL_ADDR_INCREMENT  = 1 << 2,
    CTRL_PATTERN_SPRITE  = 1 << 3,
    CTRL_PATTERN_BG      = 1 << 4,
    CTRL_SPRITE_SIZE     = 1 << 5,
    CTRL_SLAVE_MODE      = 1 << 6,
    CTRL_ENABLE_NMI      = 1 << 7
};

enum ppu_mask {
    MASK_GREYSCALE        = 1 << 0,
    MASK_SHOW_BG_LEFT     = 1 << 1,
    MASK_SHOW_SPRITE_LEFT = 1 << 2,
    MASK_SHOW_BG          = 1 << 3,
    MASK_SHOW_SPRITE      = 1 << 4,
    MASK_EMPHASIZE_R      = 1 << 5,
    MASK_EMPHASIZE_G      = 1 << 6,
    MASK_EMPHASIZE_B      = 1 << 7
};

static void set_stat(struct PPU *ppu, uint8_t flag, uint8_t val)
{
    if (val)
        ppu->stat |= flag;
    else
        ppu->stat &= ~flag;
}

static int get_ctrl(struct PPU *ppu, uint8_t flag)
{
    return (ppu->ctrl & flag) > 0;
}

static const uint8_t palette_2C02[][3] = {
    /* 0x00 */
    {0x54, 0x54, 0x54}, {0x00, 0x1E, 0x74}, {0x08, 0x10, 0x90}, {0x30, 0x00, 0x88},
    {0x44, 0x00, 0x64}, {0x5C, 0x00, 0x30}, {0x54, 0x04, 0x00}, {0x3C, 0x18, 0x00},
    {0x20, 0x2A, 0x00}, {0x08, 0x3A, 0x00}, {0x00, 0x40, 0x00}, {0x00, 0x3C, 0x00},
    {0x00, 0x32, 0x3C}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
    /* 0x10 */
    {0x98, 0x96, 0x98}, {0x08, 0x4C, 0xC4}, {0x30, 0x32, 0xEC}, {0x5C, 0x1E, 0xE4},
    {0x88, 0x14, 0xB0}, {0xA0, 0x14, 0x64}, {0x98, 0x22, 0x20}, {0x78, 0x3C, 0x00},
    {0x54, 0x5A, 0x00}, {0x28, 0x72, 0x00}, {0x08, 0x7C, 0x00}, {0x00, 0x76, 0x28},
    {0x00, 0x66, 0x78}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
    /* 0x20 */
    {0xEC, 0xEE, 0xEC}, {0x4C, 0x9A, 0xEC}, {0x78, 0x7C, 0xEC}, {0xB0, 0x62, 0xEC},
    {0xE4, 0x54, 0xEC}, {0xEC, 0x58, 0xB4}, {0xEC, 0x6A, 0x64}, {0xD4, 0x88, 0x20},
    {0xA0, 0xAA, 0x00}, {0x74, 0xC4, 0x00}, {0x4C, 0xD0, 0x20}, {0x38, 0xCC, 0x6C},
    {0x38, 0xB4, 0xCC}, {0x3C, 0x3C, 0x3C}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
    /* 0x30 */
    {0xEC, 0xEE, 0xEC}, {0xA8, 0xCC, 0xEC}, {0xBC, 0xBC, 0xEC}, {0xD4, 0xB2, 0xEC},
    {0xEC, 0xAE, 0xEC}, {0xEC, 0xAE, 0xD4}, {0xEC, 0xB4, 0xB0}, {0xE4, 0xC4, 0x90},
    {0xCC, 0xD2, 0x78}, {0xB4, 0xDE, 0x78}, {0xA8, 0xE2, 0x90}, {0x98, 0xE2, 0xB4},
    {0xA0, 0xD6, 0xE4}, {0xA0, 0xA2, 0xA0}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}
};

static const uint8_t *get_bg_palette(const uint8_t *palette, int attr)
{
    return palette + attr * 4;
}

static const uint8_t *get_color(int index)
{
    return palette_2C02[index];
}

static uint8_t get_tile_id(const struct PPU *ppu, uint8_t tile_x, uint8_t tile_y)
{
    return ppu->name_table_0[tile_y * 32 + tile_x];
}

static uint8_t get_tile_row(const struct PPU *ppu,
        uint8_t tile_id, uint8_t pixel_y, uint8_t offset)
{
    return ppu->char_rom[16 * tile_id + offset + pixel_y];
}

struct tile_cache {
    uint16_t id;
    uint8_t x, y;
    uint8_t pixel_y;
    uint8_t attr;
    uint8_t lsb, msb;
};

static void set_pixel_color(const struct PPU *ppu, int x, int y, const struct tile_cache *tile)
{
    const int pixel_x = x % 8;

    const int mask = (1 << 7) >> pixel_x;
    const uint8_t m = (tile->msb & mask) > 0;
    const uint8_t l = (tile->lsb & mask) > 0;
    const uint8_t val = (m << 1) | l;

    const uint8_t *palette = get_bg_palette(ppu->bg_palette_table, tile->attr);
    const uint8_t index = palette[val];
    const uint8_t *color = get_color(index);

    set_color(ppu->fbuf, x, y, color);
}

void clear_nmi(struct PPU *ppu)
{
    ppu->nmi_generated = 0;
}

int is_nmi_generated(const struct PPU *ppu)
{
    return ppu->nmi_generated;
}

int is_frame_ready(const struct PPU *ppu)
{
    return ppu->cycle == 0 && ppu->scanline == 0;
}

void clock_ppu(struct PPU *ppu)
{
    const int cycle = ppu->cycle;
    const int scanline = ppu->scanline;

    static struct tile_cache next_tile = {0};

    switch (cycle % 8) {
    case 1:
        /* NT byte */
        next_tile.x = (cycle - 1) / 8;
        next_tile.y = scanline    / 8;
        next_tile.pixel_y = scanline % 8;
        next_tile.id = get_tile_id(ppu, next_tile.x, next_tile.y);
        /* AT byte */
        next_tile.attr = 0;
        /* Low BG tile byte */
        next_tile.lsb = get_tile_row(ppu, next_tile.id, next_tile.pixel_y, 0);
        /* High BG tile byte */
        next_tile.msb = get_tile_row(ppu, next_tile.id, next_tile.pixel_y, 8);
        break;
    default:
        break;
    }

    if ((cycle >= 1 && cycle <= 256) &&
        (scanline >= 0 && scanline <= 239))
        set_pixel_color(ppu, cycle - 1, scanline, &next_tile);

    if (cycle == 1 && scanline == 241) {
        set_stat(ppu, STAT_VERTICAL_BLANK, 1);

        if (get_ctrl(ppu, CTRL_ENABLE_NMI))
            ppu->nmi_generated = 1;
    }

    if (cycle == 1 && scanline == 261)
        set_stat(ppu, STAT_VERTICAL_BLANK, 0);

    /* advance cycle and scanline */
    if (cycle == 340) {
        ppu->cycle    = 0;
        ppu->scanline = (scanline == 261) ? 0 : scanline + 1;
    } else {
        ppu->cycle    = cycle + 1;
    }
}

void write_ppu_control(struct PPU *ppu, uint8_t data)
{
    ppu->ctrl = data;
}

void write_ppu_mask(struct PPU *ppu, uint8_t data)
{
    ppu->mask = data;
}

void write_oam_address(struct PPU *ppu, uint8_t addr)
{
    ppu->oam_addr = addr;
}

void write_oam_data(struct PPU *ppu, uint8_t data)
{
    ppu->oam[ppu->oam_addr] = data;
}

void write_ppu_scroll(struct PPU *ppu, uint8_t data)
{
    /* TODO */
}

void write_ppu_address(struct PPU *ppu, uint8_t addr)
{
    if (ppu->addr_latch == 0)
        /* hi byte */
        ppu->ppu_addr = addr << 8;
    else
        /* lo byte */
        ppu->ppu_addr |= addr;

    ppu->addr_latch = !ppu->addr_latch;
}

void write_ppu_data(struct PPU *ppu, uint8_t data)
{
    const uint16_t addr = ppu->ppu_addr;

    ppu->ppu_data_buf = data;

    if (0x2000 <= addr && addr <= 0x23BF) {
        ppu->name_table_0[addr - 0x2000] = data;
    }
    else if (0x3F00 <= addr && addr <= 0x3F0F) {
        ppu->bg_palette_table[addr - 0x3F00] = data;
    }

    if (get_ctrl(ppu, CTRL_ADDR_INCREMENT))
        ppu->ppu_addr += 32;
    else
        ppu->ppu_addr++;
}

uint8_t read_ppu_status(struct PPU *ppu)
{
    const uint8_t data = ppu->stat;

    set_stat(ppu, STAT_VERTICAL_BLANK, 0);
    ppu->addr_latch = 0;

    return data;
}

uint8_t read_oam_data(const struct PPU *ppu)
{
    /* TODO */
    return 0;
}

uint8_t read_ppu_data(const struct PPU *ppu)
{
    /* TODO */
    return 0;
}

uint8_t peek_ppu_status(const struct PPU *ppu)
{
    return ppu->stat;
}
