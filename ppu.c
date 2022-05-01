#include <stdlib.h>
#include <stdio.h>
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

static void set_pixel_color(const struct PPU *ppu, int x, int y)
{
    const struct pattern_row patt = ppu->tile_queue[2];

    const uint8_t hi = (patt.hi & 0x80) > 0;
    const uint8_t lo = (patt.lo & 0x80) > 0;
    const uint8_t val = (hi << 1) | lo;
    const uint8_t attr = patt.attr;

    const uint8_t *palette = get_bg_palette(ppu->bg_palette_table, attr);
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

struct vram_pointer {
    uint8_t table_x, table_y;
    uint8_t tile_x, tile_y;
    uint8_t fine_y;
};

static struct vram_pointer decode_address(uint16_t addr)
{
    /*
     *  yyy NN YYYYY XXXXX
     *  ||| || ||||| +++++-- coarse X scroll
     *  ||| || +++++-------- coarse Y scroll
     *  ||| ++-------------- nametable select
     *  +++----------------- fine Y scroll
     */
    struct vram_pointer v;

    v.tile_x  = (addr     )  & 0x001F;
    v.tile_y  = (addr >> 5)  & 0x001F;
    v.table_x = (addr >> 10) & 0x0001;
    v.table_y = (addr >> 11) & 0x0001;
    v.fine_y  = (addr >> 12) & 0x0007;

    return v;
}

static uint16_t encode_address(struct vram_pointer v)
{
    /*
     *  yyy NN YYYYY XXXXX
     *  ||| || ||||| +++++-- coarse X scroll
     *  ||| || +++++-------- coarse Y scroll
     *  ||| ++-------------- nametable select
     *  +++----------------- fine Y scroll
     */
    uint16_t addr = 0;

    addr = v.fine_y & 0x07;
    addr = (addr << 1) | (v.table_y & 0x01);
    addr = (addr << 1) | (v.table_x & 0x01);
    addr = (addr << 5) | (v.tile_y & 0x1F);
    addr = (addr << 5) | (v.tile_x & 0x1F);

    return addr;
}

static void increment_scroll_x(struct PPU *ppu)
{
    struct vram_pointer v = decode_address(ppu->vram_addr);

    if (v.tile_x == 31) {
        v.tile_x = 0;
        v.table_x = !v.table_x;
    }
    else {
        v.tile_x++;
    }

    ppu->vram_addr = encode_address(v);
}

static void increment_scroll_y(struct PPU *ppu)
{
    struct vram_pointer v = decode_address(ppu->vram_addr);

    if (v.fine_y == 7) {
        v.fine_y = 0;

        if (v.tile_y == 29) {
            v.tile_y = 0;
            v.table_y = !v.table_y;
        }
        else {
            v.tile_y++;
        }
    }
    else {
        v.fine_y++;
    }

    ppu->vram_addr = encode_address(v);
}

static void copy_address_x(struct PPU *ppu)
{
    const struct vram_pointer t = decode_address(ppu->temp_addr);
    struct vram_pointer v = decode_address(ppu->vram_addr);

    v.table_x = t.table_x;
    v.tile_x  = t.tile_x;

    ppu->vram_addr = encode_address(v);
}

static void copy_address_y(struct PPU *ppu)
{
    const struct vram_pointer t = decode_address(ppu->temp_addr);
    struct vram_pointer v = decode_address(ppu->vram_addr);

    v.table_y = t.table_y;
    v.tile_y  = t.tile_y;
    v.fine_y  = t.fine_y;

    ppu->vram_addr = encode_address(v);
}

static int is_rendering_bg(const struct PPU *ppu)
{
    return ppu->mask & MASK_SHOW_BG;
}

static int is_rendering_sprite(const struct PPU *ppu)
{
    return ppu->mask & MASK_SHOW_SPRITE;
}

static void enter_vblank(struct PPU *ppu)
{
    set_stat(ppu, STAT_VERTICAL_BLANK, 1);

    if (get_ctrl(ppu, CTRL_ENABLE_NMI))
        ppu->nmi_generated = 1;
}

static void leave_vblank(struct PPU *ppu)
{
    set_stat(ppu, STAT_VERTICAL_BLANK, 0);
}

static void load_next_tile(struct PPU *ppu)
{
    ppu->tile_queue[2] = ppu->tile_queue[1];
    ppu->tile_queue[1] = ppu->tile_queue[0];
}

static void shift_tile_data(struct PPU *ppu)
{
    ppu->tile_queue[2].lo <<= 1;
    ppu->tile_queue[2].hi <<= 1;
}

static void fetch_tile_data(struct PPU *ppu, int cycle)
{
    const struct vram_pointer v = decode_address(ppu->vram_addr);
    struct pattern_row *next = &ppu->tile_queue[0];

    switch (cycle % 8) {
    case 1:
        /* NT byte */
        next->id = get_tile_id(ppu, v.tile_x, v.tile_y);
        break;

    case 3:
        /* AT byte */
        next->attr = 0;
        break;

    case 5:
        /* Low BG tile byte */
        next->lo = get_tile_row(ppu, next->id, v.fine_y, 0);
        break;

    case 7:
        /* High BG tile byte */
        next->hi = get_tile_row(ppu, next->id, v.fine_y, 8);
        break;

    default:
        break;
    }
}

void clock_ppu(struct PPU *ppu)
{
    const int cycle = ppu->cycle;
    const int scanline = ppu->scanline;
    const int is_rendering = is_rendering_bg(ppu) || is_rendering_sprite(ppu);

    if ((scanline >= 0 && scanline <= 239) || scanline == 261) {

        /* fetch bg tile */
        if ((cycle >= 1 && cycle <= 256) || (cycle >= 321 && cycle <= 336)) {
            if (cycle % 8 == 0)
                if (is_rendering)
                    increment_scroll_x(ppu);


            if (cycle % 8 == 1)
                load_next_tile(ppu);

            fetch_tile_data(ppu, cycle);
        }

        /* inc vert(v) */
        if (cycle == 256)
            if (is_rendering)
                increment_scroll_y(ppu);

        /* hori(v) = hori(t) */
        if (cycle == 257)
            if (is_rendering)
                copy_address_x(ppu);

        /* vert(v) = vert(t) */
        if ((cycle >= 280 && cycle <= 305) && scanline == 261)
            if (is_rendering)
                copy_address_y(ppu);
    }

    if (scanline == 241)
        if (cycle == 1)
            enter_vblank(ppu);

    if (scanline == 261)
        if (cycle == 1)
            leave_vblank(ppu);

    /* render pixel */
    if (scanline >= 0 && scanline <= 239)
        if (cycle >= 1 && cycle <= 256)
            if (is_rendering) {
                set_pixel_color(ppu, cycle - 1, scanline);
                shift_tile_data(ppu);
            }

    /* advance cycle and scanline */
    if (cycle == 339) {
        if (scanline == 261 && ppu->frame % 2 == 0) {
            /* the end of the scanline for odd frames */
            ppu->cycle = 0;
            ppu->scanline = 0;
            ppu->frame++;
        }
        else {
            ppu->cycle = cycle + 1;
        }
    }
    else if (cycle == 340) {
        if (scanline == 261) {
            ppu->cycle = 0;
            ppu->scanline = 0;
            ppu->frame++;
        }
        else {
            ppu->cycle = 0;
            ppu->scanline = scanline + 1;
        }
    }
    else {
        ppu->cycle = cycle + 1;
    }
}

void write_ppu_control(struct PPU *ppu, uint8_t data)
{
    /* Nametable x and y from control
     * t: ...GH.. ........ <- d: ......GH
     *    <used elsewhere> <- d: ABCDEF..
     */
    ppu->ctrl = data;
    ppu->temp_addr = (ppu->temp_addr & 0xF3FF) | ((data & 0x03) << 10);
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
    if (ppu->addr_latch == 0) {
        /* Coarse X and fine x
         * t: ....... ...ABCDE <- d: ABCDE...
         * x:              FGH <- d: .....FGH
         * w:                  <- 1
         */
        ppu->fine_x = data & 0x07;
        ppu->temp_addr = (ppu->temp_addr & 0xFFE0) | (data >> 3);
        ppu->addr_latch = 1;
    } else {
        /* Coarse Y and fine y
         * t: FGH..AB CDE..... <- d: ABCDEFGH
         * w:                  <- 0
         */
        ppu->temp_addr = (ppu->temp_addr & 0xFC1F) | ((data >> 3) << 5);
        ppu->temp_addr = (ppu->temp_addr & 0x8FFF) | (data & 0x07);
        ppu->addr_latch = 0;
    }
}

void write_ppu_address(struct PPU *ppu, uint8_t addr)
{
    if (ppu->addr_latch == 0) {
        /* High byte
         * t: .CDEFGH ........ <- d: ..CDEFGH
         *        <unused>     <- d: AB......
         * t: Z...... ........ <- 0 (bit Z is cleared)
         * w:                  <- 1
         */
        ppu->temp_addr = (ppu->temp_addr & 0xC0FF) | ((addr & 0x3F) << 8);
        ppu->temp_addr &= 0x3FFF;
        ppu->addr_latch = 1;
    } else {
        /* Low byte
         * t: ....... ABCDEFGH <- d: ABCDEFGH
         * v: <...all bits...> <- t: <...all bits...>
         * w:                  <- 0
         */
        ppu->temp_addr = (ppu->temp_addr & 0xFF00) | addr;
        ppu->vram_addr = ppu->temp_addr;
        ppu->addr_latch = 0;
    }
}

void write_ppu_data(struct PPU *ppu, uint8_t data)
{
    const uint16_t addr = ppu->vram_addr;

    ppu->ppu_data_buf = data;

    if (0x2000 <= addr && addr <= 0x23BF) {
        ppu->name_table_0[addr - 0x2000] = data;
    }
    else if (0x3F00 <= addr && addr <= 0x3F0F) {
        ppu->bg_palette_table[addr - 0x3F00] = data;
    }

    if (get_ctrl(ppu, CTRL_ADDR_INCREMENT))
        ppu->vram_addr += 32;
    else
        ppu->vram_addr++;
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
