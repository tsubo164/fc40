#include "mapper.h"
#include "mapper_000.h"
#include "mapper_002.h"
#include "mapper_003.h"

namespace nes {

Mapper::Mapper(const uint8_t *prog_rom, size_t prog_size,
        const uint8_t *char_rom, size_t char_size)
{
    prog_rom_  = prog_rom;
    prog_size_ = prog_size;
    char_rom_  = char_rom;
    char_size_ = char_size;
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

uint8_t Mapper::read_prog_rom(uint32_t addr) const
{
    if (addr < prog_size())
        return prog_rom_[addr];
    else
        return 0;
}

uint8_t Mapper::read_char_rom(uint32_t addr) const
{
    if (addr < char_size())
        return char_rom_[addr];
    else
        return 0;
}

size_t Mapper::prog_size() const
{
    return prog_size_;
}

size_t Mapper::char_size() const
{
    return char_size_;
}

std::shared_ptr<Mapper> new_mapper(int id,
        const uint8_t *prog_rom, size_t prog_size,
        const uint8_t *char_rom, size_t char_size)
{
    Mapper *m = nullptr;

    switch (id) {
    case 0:
        m = new Mapper_000(prog_rom, prog_size, char_rom, char_size);
        break;

    case 2:
        m = new Mapper_002(prog_rom, prog_size, char_rom, char_size);
        break;

    case 3:
        m = new Mapper_003(prog_rom, prog_size, char_rom, char_size);
        break;

    default:
        break;
    }

    return std::shared_ptr<Mapper>(m);
}

} // namespace
