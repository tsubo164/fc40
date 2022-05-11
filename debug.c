#include <stdint.h>
#include "debug.h"
#include "framebuffer.h"
#include "cartridge.h"

static void load_pattern(struct framebuffer *fb, const struct cartridge *cart, int id)
{
    const int tile_x = id < 256 ? id % 16 : id % 16 + 16;
    const int tile_y = id < 256 ? id / 16 : id / 16 - 16;

    const int X0 = tile_x * 8;
    const int X1 = X0 + 8;
    const int Y0 = tile_y * 8;
    const int Y1 = Y0 + 8;

    int x, y;

    for (y = Y0; y < Y1; y++) {
        const uint8_t fine_y = y - Y0;
        const uint8_t lo = read_chr_rom(cart, id * 16 + fine_y + 0);
        const uint8_t hi = read_chr_rom(cart, id * 16 + fine_y + 8);
        int mask = 1 << 7;

        for (x = X0; x < X1; x++) {
            const uint8_t l = (lo & mask) > 0;
            const uint8_t h = (hi & mask) > 0;
            const uint8_t val = (h << 1) | l;
            uint8_t color[3] = {0};
            color[0] = 255;
            color[0] = (int) (val / 3. * 255);
            color[1] = (int) (val / 3. * 255);
            color[2] = (int) (val / 3. * 255);
            set_color(fb, x, y, color);

            mask >>= 1;
        }
    }
}

void load_pattern_table(struct framebuffer *fb, const struct cartridge *cart)
{
    int i;

    for (i = 0; i < 256 * 2; i++)
        load_pattern(fb, cart, i);
}
