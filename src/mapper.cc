#include "mapper.h"
#include "mapper_000.h"
#include "mapper_001.h"
#include "mapper_002.h"
#include "mapper_003.h"
#include "mapper_004.h"
#include "mapper_010.h"
#include "mapper_016.h"
#include "mapper_019.h"
#include "mapper_076.h"

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

uint8_t Mapper::ReadNameTable(uint16_t addr) const
{
    return do_read_nametable(addr);
}

void Mapper::WritePrg(uint16_t addr, uint8_t data)
{
    do_write_prg(addr, data);
}

void Mapper::WriteChr(uint16_t addr, uint8_t data)
{
    do_write_chr(addr, data);
}

void Mapper::WriteNameTable(uint16_t addr, uint8_t data)
{
    do_write_nametable(addr, data);
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

void Mapper::GetPrgBankInfo(BankInfo &info) const
{
    do_get_prg_bank_info(info);
}

void Mapper::GetChrBankInfo(BankInfo &info) const
{
    do_get_chr_bank_info(info);
}

std::vector<uint8_t> Mapper::GetPrgRam() const
{
    return prg_ram_;
}

void Mapper::SetPrgRam(const std::vector<uint8_t> &sram)
{
    prg_ram_ = sram;
}

void Mapper::SetNameTable(std::array<uint8_t,2048> *nt)
{
    nametable_ = nt;
}

bool Mapper::HasPrgRamWritten() const
{
    return prg_ram_written_;
}

bool Mapper::IsSetIRQ() const
{
    return irq_generated_;
}

void Mapper::ClearIRQ()
{
    irq_generated_ = false;
}

void Mapper::PpuClock(int cycle, int scanline)
{
    do_ppu_clock(cycle, scanline);
}

void Mapper::CpuClock()
{
    do_cpu_clock();
}

int Mapper::GetMirroring() const
{
    return mirroring_;
}

void Mapper::SetMirroring(int mirroring)
{
    mirroring_ = mirroring;
}

std::string Mapper::GetBoardName() const
{
    return board_name_;
}

uint8_t Mapper::read_prg_rom(int index) const
{
    if (index >= 0 && index < GetPrgRomSize())
        return prg_rom_[index];
    else
        return 0xFF;
}

uint8_t Mapper::read_chr_rom(int index) const
{
    if (index >= 0 && index < GetChrRomSize())
        return chr_rom_[index];
    else
        return 0xFF;
}

uint8_t Mapper::read_prg_ram(int index) const
{
    if (index >= 0 && index < GetPrgRamSize())
        return prg_ram_[index];
    else
        return 0xFF;
}

uint8_t Mapper::read_chr_ram(int index) const
{
    if (index >= 0 && index < GetChrRamSize())
        return chr_ram_[index];
    else
        return 0xFF;
}

uint8_t Mapper::read_nametable(int index) const
{
    return (*nametable_)[index];
}

void Mapper::write_prg_ram(int index, uint8_t data)
{
    if (index >= 0 && index < GetPrgRamSize()) {
        prg_ram_[index] = data;
        prg_ram_written_ = true;
    }
}

void Mapper::write_chr_ram(int index, uint8_t data)
{
    if (index >= 0 && index < GetChrRamSize())
        chr_ram_[index] = data;
}

void Mapper::write_nametable(int index, uint8_t data)
{
    (*nametable_)[index] = data;
}

void Mapper::use_prg_ram(int size)
{
    prg_ram_.resize(size, 0x00);
}

void Mapper::use_chr_ram(int size)
{
    chr_ram_.resize(size, 0x00);
}

bool Mapper::is_prg_ram_used() const
{
    return GetPrgRamSize() > 0;
}

bool Mapper::is_chr_ram_used() const
{
    return GetChrRamSize() > 0;
}

void Mapper::set_prg_ram_protect(bool enable)
{
    prg_ram_protected_ = enable;
}

bool Mapper::is_prg_ram_protected() const
{
    return prg_ram_protected_;
}

void Mapper::set_board_name(const std::string &name)
{
    board_name_ = name;
}

void Mapper::set_irq()
{
    irq_generated_ = true;
}

uint8_t Mapper::do_read_nametable(uint16_t addr) const
{
    const int index = nametable_index(addr);
    return read_nametable(index);
}

void Mapper::do_write_nametable(uint16_t addr, uint8_t data)
{
    const int index = nametable_index(addr);
    write_nametable(index, data);
}

int Mapper::nametable_index(uint16_t addr) const
{
    if (GetMirroring() == MIRRORING_VERTICAL) {
        return addr & 0x07FF;
    }
    else if (GetMirroring() == MIRRORING_HORIZONTAL) {
        if (addr >= 0x2000 && addr <= 0x27FF)
            return addr & 0x03FF;
        if (addr >= 0x2800 && addr <= 0x2FFF)
            return 0x400 | (addr & 0x03FF);
    }
    else if (GetMirroring() == MIRRORING_SINGLE_SCREEN_0) {
        return addr & 0x03FF;
    }
    else if (GetMirroring() == MIRRORING_SINGLE_SCREEN_1) {
        return 0x400 | (addr & 0x03FF);
    }

    // unreachable
    return 0x0000;
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

    case 4:
    case 206:
        m = new Mapper_004(prg_data, chr_data);
        break;

    case 10:
        m = new Mapper_010(prg_data, chr_data);
        break;

    case 16:
        m = new Mapper_016(prg_data, chr_data);
        break;

    case 19:
        m = new Mapper_019(prg_data, chr_data);
        break;

    case 76:
        m = new Mapper_076(prg_data, chr_data);
        break;

    default:
        break;
    }

    return std::shared_ptr<Mapper>(m);
}

} // namespace
