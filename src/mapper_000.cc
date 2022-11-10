#include "mapper_000.h"

namespace nes {

Mapper_000::Mapper_000(const uint8_t *prog_rom, size_t prog_size,
        const uint8_t *char_rom, size_t char_size) :
    Mapper(prog_rom, prog_size, char_rom, char_size)
{
    prog_nbanks_ = prog_size / 0x4000; // 16KB
}

Mapper_000::~Mapper_000()
{
}

uint8_t Mapper_000::do_read_prog(uint16_t addr) const
{
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        const uint16_t a = addr & (prog_nbanks_ == 1 ? 0x3FFF : 0x7FFF);
        return read_prog_rom(a);
    }
    else {
        return 0;
    }
}

void Mapper_000::do_write_prog(uint16_t addr, uint8_t data)
{
}

uint8_t Mapper_000::do_read_char(uint16_t addr) const
{
    if (addr >= 0x0000 && addr <= 0x1FFF)
        return read_char_rom(addr);
    else
        return 0xFF;
}

void Mapper_000::do_write_char(uint16_t addr, uint8_t data)
{
}

} // namespace
