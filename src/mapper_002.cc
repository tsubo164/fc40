#include <stdio.h>
#include "mapper_002.h"

namespace nes {

Mapper_002::Mapper_002(const uint8_t *prog_rom, size_t prog_size,
        const uint8_t *char_rom, size_t char_size) :
    Mapper(prog_rom, prog_size, char_rom, char_size)
{
    prog_nbanks_ = prog_size / 0x4000; // 16KB
    char_nbanks_ = char_size / 0x2000; // 8KB

    prog_bank_ = 0;
    prog_fixed_ = prog_nbanks_ - 1;
}

Mapper_002::~Mapper_002()
{
}

uint8_t Mapper_002::do_read_prog(uint16_t addr) const
{
    if (addr >= 0x8000 && addr <= 0xBFFF) {
        const uint32_t a = prog_bank_ * 0x4000 + (addr & 0x3FFF);
        return prog_rom_[a];
    }
    else if (addr >= 0xC000 && addr <= 0xFFFF) {
        const uint32_t a = prog_fixed_ * 0x4000 + (addr & 0x3FFF);
        return prog_rom_[a];
    }
    else {
        return 0;
    }
}

void Mapper_002::do_write_prog(uint16_t addr, uint8_t data)
{
    if (addr >= 0x8000 && addr <= 0xFFFF)
        prog_bank_ = data & 0x0F;
}

uint8_t Mapper_002::do_read_char(uint16_t addr) const
{
    if (addr >= 0x0000 && addr <= 0x1FFF)
        return char_ram_[addr];
    else
        return 0xFF;
}

void Mapper_002::do_write_char(uint16_t addr, uint8_t data)
{
    if (addr >= 0x0000 && addr <= 0x1FFF)
        char_ram_[addr] = data;
}

} // namespace
