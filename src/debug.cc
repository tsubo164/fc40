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

void print_cpu_status_style_1(const CPU &cpu, const PPU &ppu)
{
    const CpuStatus stat = cpu.GetStatus();
    const Code line = DisassembleLine(cpu, stat.pc);
    const std::string code_str = GetCodeString(line);
    const std::string mem_str = GetMemoryString(line, cpu);

    // Registers
    printf("A:%02X X:%02X Y:%02X S:%02X ", stat.a, stat.x, stat.y, stat.s);
    // Flags
    char flags[] = "nvubdizc";
    for (int i = 0; i < 8; i++)
        if (stat.p & (0x80 >> i))
            flags[i] = std::toupper(flags[i]);
    printf("P:%s ", flags);

    // Stack Pointer
    const int depth = 0xFF - stat.s;
    for (int i = 0; i < depth; i++) {
        printf(" ");
    }

    // Code
    printf("$%s:%s%s",
            code_str.substr(0, 4).c_str(),
            code_str.substr(5, 10).c_str(),
            code_str.substr(16).c_str());

    // Memory ref
    std::string memref = mem_str;
    {
        const auto pos = memref.find(" @ ");
        if (pos != std::string::npos) {
            memref =
                memref.substr(0, pos + 3) + "$" +
                memref.substr(pos + 3);
        }
    }
    {
        const auto pos = memref.find(" = ");
        if (pos != std::string::npos) {
            memref =
                memref.substr(0, pos + 3) + "#$" +
                memref.substr(pos + 3);
        }
    }
    printf("%s", memref.c_str());

    printf("\n");
}

void LogCpuStatus(NES &nes, int max_lines)
{
    InitSound();
    nes.StartLog();
    nes.cpu.SetPC(0xC000);

    const int cpu_cycles = nes.cpu.GetTotalCycles();
    nes.ppu.Run(cpu_cycles);

    while (nes.GetLogLineCount() < max_lines) {
        nes.UpdateFrame();
    }
}

static void load_pattern(FrameBuffer &fb, const Cartridge *cart, int tile_id, int table_cell)
{
    const int tile_x = table_cell < 256 ? table_cell % 16 : table_cell % 16 + 16;
    const int tile_y = table_cell < 256 ? table_cell / 16 : table_cell / 16 - 16;

    const int X0 = tile_x * 8;
    const int X1 = X0 + 8;
    const int Y0 = tile_y * 8;
    const int Y1 = Y0 + 8;

    for (int y = Y0; y < Y1; y++) {
        const uint8_t fine_y = y - Y0;
        const uint8_t lo = cart->ReadChr(tile_id * 16 + fine_y + 0);
        const uint8_t hi = cart->ReadChr(tile_id * 16 + fine_y + 8);
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

static void load_sprite(FrameBuffer &fb, const PPU &ppu, int tile_id, int table_cell)
{
    const int tile_x = table_cell < 256 ? table_cell % 16 : table_cell % 16 + 16;
    const int tile_y = table_cell < 256 ? table_cell / 16 : table_cell / 16 - 16;

    const int X0 = tile_x * 8;
    const int X1 = X0 + 8;
    const int Y0 = tile_y * 8;
    const int Y1 = Y0 + 8;

    for (int y = Y0; y < Y1; y++) {
        const uint8_t fine_y = y - Y0;
        const uint8_t lo = ppu.GetSpriteRow(tile_id, fine_y, 0);
        const uint8_t hi = ppu.GetSpriteRow(tile_id, fine_y, 8);
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
        load_pattern(fb, cart, i, i);
}

void LoadOamTable(FrameBuffer &fb, const PPU &ppu)
{
    for (int i = 0; i < 64; i++) {
        const ObjectAttribute obj = ppu.ReadOam(i);
        load_sprite(fb, ppu, obj.tile_id, i);
    }
}

} // namespace
