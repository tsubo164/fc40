#include "mapper.h"
#include "mapper_000.h"
#include "mapper_001.h"
#include "mapper_002.h"
#include "mapper_003.h"

namespace nes {

Mapper::Mapper(const uint8_t *prog_rom, size_t prog_size,
        const uint8_t *char_rom, size_t char_size)
{
}

Mapper::Mapper(const std::vector<uint8_t> &prog_data,
        const std::vector<uint8_t> &char_data) :
    prog_rom_(prog_data), char_rom_(char_data)
{
}

Mapper::~Mapper()
{
}

void Mapper::LoadProgData(const std::vector<uint8_t> &data)
{
    prog_rom_ = data;
}

void Mapper::LoadCharData(const std::vector<uint8_t> &data)
{
    char_rom_ = data;
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

uint8_t Mapper::PeekProg(uint32_t physical_addr) const
{
    if (physical_addr < GetProgRomSize())
        return prog_rom_[physical_addr];
    else
        return 0;
}

size_t Mapper::GetProgRomSize() const
{
    return prog_rom_.size();
}

size_t Mapper::GetCharRomSize() const
{
    return char_rom_.size();
}

uint8_t Mapper::read_prog_rom(uint32_t addr) const
{
    if (addr < GetProgRomSize())
        return prog_rom_[addr];
    else
        return 0;
}

uint8_t Mapper::read_char_rom(uint32_t addr) const
{
    if (addr < GetCharRomSize())
        return char_rom_[addr];
    else
        return 0;
}

std::shared_ptr<Mapper> new_mapper(int id,
        const uint8_t *prog_rom, size_t prog_size,
        const uint8_t *char_rom, size_t char_size)
{
    Mapper *m = nullptr;
    std::vector<uint8_t> prog_rom_(prog_size);
    std::vector<uint8_t> char_rom_(char_size);
    std::copy(prog_rom, prog_rom + prog_size, prog_rom_.begin());
    std::copy(char_rom, char_rom + char_size, char_rom_.begin());

    switch (id) {
    case 0:
        m = new Mapper_000(prog_rom_, char_rom_);
        break;

    case 1:
        m = new Mapper_001(prog_rom, prog_size, char_rom, char_size);
        break;

    case 2:
        m = new Mapper_002(prog_rom, prog_size, char_rom, char_size);
        break;

    case 3:
        m = new Mapper_003(prog_rom_, char_rom_);
        break;

    default:
        break;
    }

    return std::shared_ptr<Mapper>(m);
}

} // namespace
