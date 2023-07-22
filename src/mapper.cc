#include "mapper.h"
#include "mapper_000.h"
#include "mapper_001.h"
#include "mapper_002.h"
#include "mapper_003.h"

namespace nes {

Mapper::Mapper(
        const std::vector<uint8_t> &prg_data,
        const std::vector<uint8_t> &chr_data) :
    prg_rom_(prg_data), chr_rom_(chr_data)
{
}

Mapper::~Mapper()
{
}

uint8_t Mapper::ReadPrg(uint16_t addr) const
{
    return do_read_prg(addr);
}

uint8_t Mapper::ReadChr(uint16_t addr) const
{
    return do_read_chr(addr);
}

void Mapper::WritePrg(uint16_t addr, uint8_t data)
{
    do_write_prg(addr, data);
}

void Mapper::WriteChr(uint16_t addr, uint8_t data)
{
    do_write_chr(addr, data);
}

uint8_t Mapper::PeekPrg(uint32_t physical_addr) const
{
    if (physical_addr < GetPrgRomSize())
        return prg_rom_[physical_addr];
    else
        return 0xFF;
}

size_t Mapper::GetPrgRomSize() const
{
    return prg_rom_.size();
}

size_t Mapper::GetChrRomSize() const
{
    return chr_rom_.size();
}

size_t Mapper::GetPrgRamSize() const
{
    return prg_ram_.size();
}

size_t Mapper::GetChrRamSize() const
{
    return chr_ram_.size();
}

std::vector<uint8_t> Mapper::GetPrgRam() const
{
    return prg_ram_;
}

void Mapper::SetPrgRam(const std::vector<uint8_t> &sram)
{
    prg_ram_ = sram;
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

uint8_t Mapper::read_prg_rom(uint32_t index) const
{
    if (index < GetPrgRomSize())
        return prg_rom_[index];
    else
        return 0xFF;
}

uint8_t Mapper::read_chr_rom(uint32_t index) const
{
    if (index < GetChrRomSize())
        return chr_rom_[index];
    else
        return 0xFF;
}

uint8_t Mapper::read_prg_ram(uint32_t index) const
{
    if (index < GetPrgRamSize())
        return prg_ram_[index];
    else
        return 0xFF;
}

uint8_t Mapper::read_chr_ram(uint32_t index) const
{
    if (index < GetChrRamSize())
        return chr_ram_[index];
    else
        return 0xFF;
}

void Mapper::write_prg_ram(uint32_t index, uint8_t data)
{
    if (index < GetPrgRamSize())
        prg_ram_[index] = data;
}

void Mapper::write_chr_ram(uint32_t index, uint8_t data)
{
    if (index < GetChrRamSize())
        chr_ram_[index] = data;
}

void Mapper::use_prg_ram(uint32_t size)
{
    prg_ram_.resize(size, 0x00);
}

void Mapper::use_chr_ram(uint32_t size)
{
    chr_ram_.resize(size, 0x00);
}

void Mapper::set_board_name(const std::string &name)
{
    board_name_ = name;
}

std::shared_ptr<Mapper> new_mapper(int id,
        const std::vector<uint8_t> &prg_data,
        const std::vector<uint8_t> &chr_data)
{
    Mapper *m = nullptr;

    switch (id) {
    case 0:
        m = new Mapper_000(prg_data, chr_data);
        break;

    case 1:
        m = new Mapper_001(prg_data, chr_data);
        break;

    case 2:
        m = new Mapper_002(prg_data, chr_data);
        break;

    case 3:
        m = new Mapper_003(prg_data, chr_data);
        break;

    default:
        break;
    }

    return std::shared_ptr<Mapper>(m);
}

} // namespace
