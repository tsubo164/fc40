#include "mapper.h"
#include "mapper_000.h"
#include "mapper_001.h"
#include "mapper_002.h"
#include "mapper_003.h"

namespace nes {

Mapper::Mapper(const std::vector<uint8_t> &prog_data,
        const std::vector<uint8_t> &char_data) :
    prog_rom_(prog_data), char_rom_(char_data)
{
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

size_t Mapper::GetProgRamSize() const
{
    return prog_ram_.size();
}

size_t Mapper::GetCharRamSize() const
{
    return char_ram_.size();
}

std::vector<uint8_t> Mapper::GetProgRam() const
{
    return prog_ram_;
}

void Mapper::SetProgRam(const std::vector<uint8_t> &sram)
{
    prog_ram_ = sram;
}

Mirroring Mapper::GetMirroring() const
{
    return mirroring_;
}

void Mapper::SetMirroring(Mirroring mirroring)
{
    mirroring_ = mirroring;
}

std::string Mapper::GetBoardName() const
{
    return board_name_;
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

uint8_t Mapper::read_prog_ram(uint32_t addr) const
{
    if (addr < GetProgRamSize())
        return prog_ram_[addr];
    else
        return 0;
}

uint8_t Mapper::read_char_ram(uint32_t addr) const
{
    if (addr < GetCharRamSize())
        return char_ram_[addr];
    else
        return 0;
}

void Mapper::write_prog_ram(uint32_t addr, uint8_t data)
{
    if (addr < GetProgRamSize())
        prog_ram_[addr] = data;
}

void Mapper::write_char_ram(uint32_t addr, uint8_t data)
{
    if (addr < GetCharRamSize())
        char_ram_[addr] = data;
}

void Mapper::use_prog_ram(uint32_t size)
{
    prog_ram_.resize(size, 0x00);
}

void Mapper::use_char_ram(uint32_t size)
{
    char_ram_.resize(size, 0x00);
}

void Mapper::set_board_name(const std::string &name)
{
    board_name_ = name;
}

std::shared_ptr<Mapper> new_mapper(int id,
        const std::vector<uint8_t> &prog_data,
        const std::vector<uint8_t> &char_data)
{
    Mapper *m = nullptr;

    switch (id) {
    case 0:
        m = new Mapper_000(prog_data, char_data);
        break;

    case 1:
        m = new Mapper_001(prog_data, char_data);
        break;

    case 2:
        m = new Mapper_002(prog_data, char_data);
        break;

    case 3:
        m = new Mapper_003(prog_data, char_data);
        break;

    default:
        break;
    }

    return std::shared_ptr<Mapper>(m);
}

} // namespace
