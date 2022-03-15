#include <stdlib.h>
#include "ppu.h"
#include "framebuffer.h"

static uint16_t ppu_addr;
static uint8_t ppu_data;
static uint8_t bg_palette_table[16] = {0};
static uint8_t name_table_0[0x03C0] = {0};

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

static const uint8_t *get_bg_palette(int attr)
{
    return bg_palette_table + attr * 4;
}

static const uint8_t *get_color(int index)
{
    return palette_2C02[index];
}

static void set_pixel_color(struct framebuffer *fb, const uint8_t *chr, int x, int y)
{
    uint8_t *table = name_table_0;

    const int namex = x / 8;
    const int namey = y / 8;
    const uint8_t data = table[namey * 32 + namex];

    const int i = x % 8;
    const int j = y % 8;

    uint8_t lsb = chr[16 * data + j];
    uint8_t msb = chr[16 * data + 8 + j];

    const int mask = 1 << (7 - i);
    uint8_t val = 0;

    lsb = (lsb & mask) > 0;
    msb = (msb & mask) > 0;
    val = (msb << 1) | lsb;

    {
        uint8_t color[3] = {64, 0, 0};
        set_color(fb, x, y, color);
    }
    if (val) {
        const uint8_t *palette = get_bg_palette(0);
        const uint8_t index = palette[val];
        const uint8_t *color = get_color(index);

        set_color(fb, x, y, color);
    }
}

void clock_ppu(struct PPU *ppu)
{
    if (ppu->y < 240)
        set_pixel_color(ppu->fbuf, ppu->char_rom, ppu->x, ppu->y);

    ppu->x++;
    if (ppu->x == 256) {
        ppu->x = 0;
        ppu->y++;
    }
    if (ppu->y == 240) {
        ppu->y = 0;
        clear_color(ppu->fbuf);
    }
}

void write_ppu_addr(uint8_t hi_or_lo)
{
    static int is_high = 1;
    uint8_t data = hi_or_lo;

    ppu_addr = is_high ? data << 8 : ppu_addr + data;
    is_high = !is_high;
}

void write_ppu_data(uint8_t data)
{
    ppu_data = data;

    if (0x2000 <= ppu_addr && ppu_addr <= 0x23BF) {
        name_table_0[ppu_addr - 0x2000] = data;
        ppu_addr++;
    }
    if (0x3F00 <= ppu_addr && ppu_addr <= 0x3F0F) {
        bg_palette_table[ppu_addr - 0x3F00] = data;
        ppu_addr++;
    }
}
