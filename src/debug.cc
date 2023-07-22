#include <cstdint>
#include <string>
#include "debug.h"
#include "disassemble.h"
#include "framebuffer.h"
#include "cartridge.h"
#include "sound.h"
#include "nes.h"
#include "cpu.h"
#include "ppu.h"

namespace nes {

void PrintCpuStatus(const CPU &cpu, const PPU &ppu)
{
    const CpuStatus stat = cpu.GetStatus();
    const Code line = DisassembleLine(cpu, stat.pc);
    const std::string code_str = GetCodeString(line);
    const std::string mem_str = GetMemoryString(line, cpu);
    const int padding = 48 - code_str.length() - mem_str.length();

    printf("%s%s", code_str.c_str(), mem_str.c_str());
    printf("%*s", padding, " ");
    printf("A:%02X X:%02X Y:%02X P:%02X SP:%02X",
            stat.a, stat.x, stat.y, stat.p, stat.s);

    printf(" PPU:%3d,%3d", ppu.GetScanline(), ppu.GetCycle());

    printf(" CYC:%llu", cpu.GetTotalCycles());
    printf("\n");
}

void LogCpuStatus(NES &nes, int max_lines)
{
    InitSound();
    nes.StartLog();
    nes.cpu.SetPC(0xC000);

    const int cpu_cycles = nes.cpu.GetTotalCycles();
    nes.ppu.Run(cpu_cycles);

    for (int i = 0; i < max_lines; i++) {
        nes.UpdateFrame();
    }
}

static void load_pattern(FrameBuffer &fb, const Cartridge *cart, int id)
{
    const int tile_x = id < 256 ? id % 16 : id % 16 + 16;
    const int tile_y = id < 256 ? id / 16 : id / 16 - 16;

    const int X0 = tile_x * 8;
    const int X1 = X0 + 8;
    const int Y0 = tile_y * 8;
    const int Y1 = Y0 + 8;

    for (int y = Y0; y < Y1; y++) {
        const uint8_t fine_y = y - Y0;
        const uint8_t lo = cart->ReadChr(id * 16 + fine_y + 0);
        const uint8_t hi = cart->ReadChr(id * 16 + fine_y + 8);
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

void LoadPatternTable(FrameBuffer &fb, const Cartridge *cart)
{
    for (int i = 0; i < 256 * 2; i++)
        load_pattern(fb, cart, i);
}

} // namespace
