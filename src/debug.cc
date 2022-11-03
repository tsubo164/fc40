#include <cstdint>
#include "debug.h"
#include "framebuffer.h"
#include "cartridge.h"
#include "cpu.h"
#include "log.h"

namespace nes {

void LogCpuStatus(struct CPU *cpu, int max_lines)
{
    uint16_t log_line = 0;
    cpu->pc = 0xC000;

    while (log_line < max_lines) {
        if (cpu->cycles == 0) {
            PrintCpuStatus(cpu);
            log_line++;
        }
        clock_cpu(cpu);
    }
}

static void load_pattern(FrameBuffer &fb, const struct cartridge *cart, int id)
{
    const int tile_x = id < 256 ? id % 16 : id % 16 + 16;
    const int tile_y = id < 256 ? id / 16 : id / 16 - 16;

    const int X0 = tile_x * 8;
    const int X1 = X0 + 8;
    const int Y0 = tile_y * 8;
    const int Y1 = Y0 + 8;

    for (int y = Y0; y < Y1; y++) {
        const uint8_t fine_y = y - Y0;
        const uint8_t lo = cart->ReadChar(id * 16 + fine_y + 0);
        const uint8_t hi = cart->ReadChar(id * 16 + fine_y + 8);
        int mask = 1 << 7;

        for (int x = X0; x < X1; x++) {
            const uint8_t l = (lo & mask) > 0;
            const uint8_t h = (hi & mask) > 0;
            const uint8_t val = (h << 1) | l;
            const Color col = {
                static_cast<uint8_t>(val / 3. * 255),
                static_cast<uint8_t>(val / 3. * 255),
                static_cast<uint8_t>(val / 3. * 255)
            };

            fb.SetColor(x, y, col);

            mask >>= 1;
        }
    }
}

void LoadPatternTable(FrameBuffer &fb, const struct cartridge *cart)
{
    for (int i = 0; i < 256 * 2; i++)
        load_pattern(fb, cart, i);
}

} // namespace
