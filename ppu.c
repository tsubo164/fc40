#include <stdlib.h>
#include <stdio.h>
#include "ppu.h"
#include "framebuffer.h"
#include "cartridge.h"

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

static int get_ctrl(const struct PPU *ppu, uint8_t flag)
{
    return (ppu->ctrl & flag) > 0;
}

static const uint8_t palette_2C02[][3] = {
    { 84,  84,  84}, {  0,  30, 116}, {  8,  16, 144}, { 48,   0, 136},
    { 68,   0, 100}, { 92,   0,  48}, { 84,   4,   0}, { 60,  24,   0},
    { 32,  42,   0}, {  8,  58,   0}, {  0,  64,   0}, {  0,  60,   0},
    {  0,  50,  60}, {  0,   0,   0}, {  0,   0,   0}, {  0,   0,   0},

    {152, 150, 152}, {  8,  76, 196}, { 48,  50, 236}, { 92,  30, 228},
    {136,  20, 176}, {160,  20, 100}, {152,  34,  32}, {120,  60,   0},
    { 84,  90,   0}, { 40, 114,   0}, {  8, 124,   0}, {  0, 118,  40},
    { 0 ,102 ,120 }, {  0,   0,   0}, {  0,   0,   0}, {  0,   0,   0},

    {236, 238, 236}, { 76, 154, 236}, {120, 124, 236}, {176,  98, 236},
    {228,  84, 236}, {236,  88, 180}, {236, 106, 100}, {212, 136,  32},
    {160, 170,   0}, {116, 196,   0}, { 76, 208,  32}, { 56, 204, 108},
    {56 ,180 ,204 }, { 60,  60,  60}, {  0,   0,   0}, {  0,   0,   0},

    {236, 238, 236}, {168, 204, 236}, {188, 188, 236}, {212, 178, 236},
    {236, 174, 236}, {236, 174, 212}, {236, 180, 176}, {228, 196, 144},
    {204, 210, 120}, {180, 222, 120}, {168, 226, 144}, {152, 226, 180},
    {160, 214, 228}, {160, 162, 160}, {  0,   0,   0}, {  0,   0,   0}
};

static const uint8_t *get_color(int index)
{
    return palette_2C02[index];
}

static uint8_t read_byte(const struct PPU *ppu, uint16_t addr)
{
    if (addr >= 0x2000 && addr <= 0x23FF) {
        return ppu->name_table_0[addr - 0x2000];
    }
    else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        const uint16_t a = 0x3F00 + (addr & 0x1F);
        /* Addresses $3F04/$3F08/$3F0C can contain unique data,
         * though these values are not used by the PPU when normally rendering
         * (since the pattern values that would otherwise select those cells
         * select the backdrop color instead). */
        if (a % 4 == 0)
            return ppu->palette_ram[0x00];
        else
            return ppu->palette_ram[a & 0x1F];
    }

    return 0xFF;
}

static void write_byte(struct PPU *ppu, uint16_t addr, uint8_t data)
{
    if (0x2000 <= addr && addr <= 0x23FF) {
        ppu->name_table_0[addr - 0x2000] = data;
    }
    else if (0x3F00 <= addr && addr <= 0x3FFF) {
        /* $3F20-$3FFF -> Mirrors of $3F00-$3F1F */
        uint16_t a = 0x3F00 + (addr & 0x1F);

        /* Addresses $3F10/$3F14/$3F18/$3F1C are mirrors of
         * $3F00/$3F04/$3F08/$3F0C. */
        if (addr == 0x3F10) a = 0x3F00;
        if (addr == 0x3F14) a = 0x3F04;
        if (addr == 0x3F18) a = 0x3F08;
        if (addr == 0x3F1C) a = 0x3F0C;

        ppu->palette_ram[a & 0x1F] = data;
    }
}

static uint8_t fetch_palette_value(const struct PPU *ppu, uint8_t palette_id, uint8_t pixel_val)
{
    const uint16_t addr = 0x3F00 + 4 * palette_id + pixel_val;

    return read_byte(ppu, addr);
}

static void set_pixel_color(const struct PPU *ppu, int x, int y)
{
    const struct pattern_row patt = ppu->tile_queue[2];

    const uint8_t hi = (patt.hi & 0x80) > 0;
    const uint8_t lo = (patt.lo & 0x80) > 0;
    const uint8_t val = (hi << 1) | lo;
    const uint8_t attr_lo = (patt.attr_lo & 0x80) > 0;
    const uint8_t attr_hi = (patt.attr_hi & 0x80) > 0;
    const uint8_t attr = (attr_hi << 1) | attr_lo;

    const uint8_t index = fetch_palette_value(ppu, attr, val);
    const uint8_t *color = get_color(index);

    set_color(ppu->fbuf, x, y, color);

    if (0) {
    const uint8_t lo2 = (ppu->tile_queue_lo & 0x8000) > 0;
    const uint8_t hi2 = (ppu->tile_queue_hi & 0x8000) > 0;
    const uint8_t val2 = (hi2 << 1) | lo2;
    const uint8_t attr_lo = (ppu->tile_queue_attr_lo & 0x8000) > 0;
    const uint8_t attr_hi = (ppu->tile_queue_attr_hi & 0x8000) > 0;
    const uint8_t attr2 = (attr_hi << 1) | attr_lo;

    const uint8_t index2 = fetch_palette_value(ppu, attr2, val2);
    const uint8_t *color2 = get_color(index2);
    set_color(ppu->fbuf, x, y, color2);
    }
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

static uint8_t fetch_tile_id(const struct PPU *ppu)
{
    if (1) {
        const struct vram_pointer v = decode_address(ppu->vram_addr);
        const uint16_t offset = v.tile_y * 32 + v.tile_x;

        return read_byte(ppu, 0x2000 + offset);
    }
    else {
        return read_byte(ppu, 0x2000 + (ppu->vram_addr & 0x0FFF));
    }
}

static uint8_t fetch_tile_attr(const struct PPU *ppu)
{
    const struct vram_pointer v = decode_address(ppu->vram_addr);
    const uint16_t attr_x = v.tile_x / 4;
    const uint16_t attr_y = v.tile_y / 4;
    const uint16_t offset = attr_y * 8 + attr_x;
    const uint8_t attr = read_byte(ppu, 0x2000 + 32 * 30 + offset);

    const uint8_t bit_x = v.tile_x % 4 > 1;
    const uint8_t bit_y = v.tile_y % 4 > 1;
    const uint8_t bit = (bit_y << 1) | bit_x;

    return (attr >> (bit * 2)) & 0x03;
}

static uint8_t fetch_tile_row(const struct PPU *ppu, uint8_t tile_id, uint8_t plane)
{
    const struct vram_pointer v = decode_address(ppu->vram_addr);
    const uint16_t base = get_ctrl(ppu, CTRL_PATTERN_BG) ? 0x1000 : 0x0000;
    const uint16_t addr = base + 16 * tile_id + plane + v.fine_y;

    return read_chr_rom(ppu->cart, addr);
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

static void load_byte(uint16_t *word, uint8_t byte)
{
    *word = (*word & 0xFF00) | byte;
}

static void load_next_tile(struct PPU *ppu)
{
    ppu->tile_queue[1] = ppu->tile_queue[0];

    if (0) {
    load_byte(&ppu->tile_queue_lo, ppu->tile_next_lo);
    load_byte(&ppu->tile_queue_hi, ppu->tile_next_hi);
    load_byte(&ppu->tile_queue_attr_lo, (ppu->tile_next_attr & 1) ? 0xFF : 0x00);
    load_byte(&ppu->tile_queue_attr_hi, (ppu->tile_next_attr & 2) ? 0xFF : 0x00);
    }
}

static void shift_word(uint8_t *left, uint8_t *right)
{
    *left <<= 1;
    *left |= (*right & 0x80) > 0;
    *right <<= 1;
}

static void shift_tile_data(struct PPU *ppu)
{
    shift_word(&ppu->tile_queue[2].lo, &ppu->tile_queue[1].lo);
    shift_word(&ppu->tile_queue[2].hi, &ppu->tile_queue[1].hi);
    shift_word(&ppu->tile_queue[2].attr_lo, &ppu->tile_queue[1].attr_lo);
    shift_word(&ppu->tile_queue[2].attr_hi, &ppu->tile_queue[1].attr_hi);

    if (0) {
    ppu->tile_queue_lo <<= 1;
    ppu->tile_queue_hi <<= 1;
    ppu->tile_queue_attr_lo <<= 1;
    ppu->tile_queue_attr_hi <<= 1;
    }
}

static void fetch_tile_data(struct PPU *ppu, int cycle)
{
    struct pattern_row *next = &ppu->tile_queue[0];
    uint8_t attr;

    switch (cycle % 8) {
    case 1:
        /* NT byte */
        next->id = fetch_tile_id(ppu);
        if (0)
        ppu->tile_next_id = fetch_tile_id(ppu);
        break;

    case 3:
        /* AT byte */
        attr = fetch_tile_attr(ppu);
        next->attr_lo = (attr & 0x01) ? 0xFF : 0x00;
        next->attr_hi = (attr & 0x02) ? 0xFF : 0x00;
        if (0)
        ppu->tile_next_attr = fetch_tile_attr(ppu);
        break;

    case 5:
        /* Low BG tile byte */
        next->lo = fetch_tile_row(ppu, next->id, 0);
        if (0)
        ppu->tile_next_lo = fetch_tile_row(ppu, ppu->tile_next_id, 0);
        break;

    case 7:
        /* High BG tile byte */
        next->hi = fetch_tile_row(ppu, next->id, 8);
        if (0)
        ppu->tile_next_hi = fetch_tile_row(ppu, ppu->tile_next_id, 8);
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
    write_byte(ppu, ppu->vram_addr, data);

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
    return ppu->oam[ppu->oam_addr];
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
