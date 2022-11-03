#include "mapper.h"
#include "mapper_002.h"

namespace nes {

Mapper::Mapper(const uint8_t *prog_rom, size_t prog_size,
        const uint8_t *char_rom, size_t char_size)
{
    prog_rom_  = prog_rom;
    prog_size_ = prog_size;
    char_rom_  = char_rom;
    char_size_ = char_size;

    prog_nbanks_ = prog_size_ / 0x4000; // 16KB
    char_nbanks_ = char_size_ / 0x2000; // 8KB
}

Mapper::~Mapper()
{
}

uint8_t Mapper::ReadProg(uint16_t addr) const
{
    return do_read_prog(addr);
}

void Mapper::WriteProg(uint16_t addr, uint8_t data)
{
    do_write_prog(addr, data);
}

uint8_t Mapper::ReadChar(uint16_t addr) const
{
    return do_read_char(addr);
}

void Mapper::WriteChar(uint16_t addr, uint8_t data)
{
    do_write_char(addr, data);
}

uint8_t Mapper::do_read_prog(uint16_t addr) const
{
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        const uint16_t a = addr & (prog_nbanks_ == 1 ? 0x3FFF : 0x7FFF);
        return prog_rom_[a];
    }
    else {
        return 0;
    }
}

void Mapper::do_write_prog(uint16_t addr, uint8_t data)
{
}

uint8_t Mapper::do_read_char(uint16_t addr) const
{
    if (addr >= 0x0000 && addr <= 0x1FFF)
        return char_rom_[addr];
    else
        return 0xFF;
}

void Mapper::do_write_char(uint16_t addr, uint8_t data)
{
}

Mapper *open_mapper(int id,
        const uint8_t *prog_rom, size_t prog_size,
        const uint8_t *char_rom, size_t char_size)
{
    switch (id) {
    case 0:
        return new Mapper(prog_rom, prog_size, char_rom, char_size);

    case 2:
        return new Mapper_002(prog_rom, prog_size, char_rom, char_size);

    default:
        return nullptr;
    }
}

void close_mapper(Mapper *m)
{
    delete m;
}

} // namespace
