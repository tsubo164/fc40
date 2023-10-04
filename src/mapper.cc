#include "mapper.h"
#include "mapper_000.h"
#include "mapper_001.h"
#include "mapper_002.h"
#include "mapper_003.h"
#include "mapper_004.h"
#include "mapper_019.h"

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

uint8_t Mapper::read_prg_rom(uint32_t addr) const
{
    if (new_api) {
    const uint32_t index = prg_banks_.Map(addr - 0x8000);
    if (index < GetPrgRomSize())
        return prg_rom_[index];
    else
        return 0xFF;
    }

    if (addr < GetPrgRomSize())
        return prg_rom_[addr];
    else
        return 0xFF;
}

uint8_t Mapper::read_chr_rom(uint32_t addr) const
{
    if (new_api) {
    const uint32_t index = chr_banks_.Map(addr);
    if (index < GetChrRomSize())
        return chr_rom_[index];
    else
        return 0xFF;
    }

    if (addr < GetChrRomSize())
        return chr_rom_[addr];
    else
        return 0xFF;
}

uint8_t Mapper::read_prg_ram(uint32_t addr) const
{
    if (new_api) {
    const uint32_t index = prg_banks_.Map(addr - 0x6000);
    if (index < GetPrgRamSize())
        return prg_ram_[index];
    else
        return 0xFF;
    }

    if (addr < GetPrgRamSize())
        return prg_ram_[addr];
    else
        return 0xFF;
}

uint8_t Mapper::read_chr_ram(uint32_t addr) const
{
    if (new_api) {
    const uint32_t index = chr_banks_.Map(addr);
    if (index < GetChrRamSize())
        return chr_ram_[index];
    else
        return 0xFF;
    }

    if (addr < GetChrRamSize())
        return chr_ram_[addr];
    else
        return 0xFF;
}

uint8_t Mapper::read_nametable(uint16_t addr) const
{
    const uint32_t index = addr - 0x2000;
    return (*nametable_)[index];
}

void Mapper::write_prg_ram(uint32_t addr, uint8_t data)
{
    if (new_api) {
    const uint32_t index = prg_banks_.Map(addr - 0x6000);
    if (index < GetPrgRamSize())
        prg_ram_[index] = data;
    }

    if (addr < GetPrgRamSize())
        prg_ram_[addr] = data;
}

void Mapper::write_chr_ram(uint32_t addr, uint8_t data)
{
    if (new_api) {
    const uint32_t index = chr_banks_.Map(addr);
    if (index < GetChrRamSize())
        chr_ram_[index] = data;
    }

    if (addr < GetChrRamSize())
        chr_ram_[addr] = data;
}

void Mapper::write_nametable(uint16_t addr, uint8_t data)
{
    const uint32_t index = addr - 0x2000;
    (*nametable_)[index] = data;
}

void Mapper::use_prg_ram(uint32_t size)
{
    prg_ram_.resize(size, 0x00);
}

void Mapper::use_chr_ram(uint32_t size)
{
    chr_ram_.resize(size, 0x00);
}

void Mapper::set_prg_bank_size(Size bank_size)
{
    new_api = true;
    const uint32_t window_size = static_cast<uint16_t>(Size::_32KB);
    const uint16_t window_count =
        window_size / static_cast<uint16_t>(bank_size);

    prg_banks_.Resize(GetPrgRomSize(), bank_size, window_count);
}

void Mapper::set_chr_bank_size(Size bank_size)
{
    new_api = true;
    const uint32_t window_size = static_cast<uint16_t>(Size::_8KB);
    const uint16_t window_count =
        window_size / static_cast<uint16_t>(bank_size);

    chr_banks_.Resize(GetPrgRomSize(), bank_size, window_count);
}

void Mapper::set_chr_bank_size(Size bank_size, uint16_t window_count)
{
    new_api = true;
    chr_banks_.Resize(GetPrgRomSize(), bank_size, window_count);
}

void Mapper::select_prg_bank(uint16_t window_base, int16_t bank_index)
{
    const uint16_t window_index =
        (window_base - 0x8000) / prg_banks_.GetBankSize();

    prg_banks_.Select(window_index, bank_index);
}

void Mapper::select_chr_bank(uint16_t window_base, int16_t bank_index)
{
    const uint16_t window_index =
        window_base / chr_banks_.GetBankSize();

    chr_banks_.Select(window_index, bank_index);
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
    const uint16_t index = nametable_index(addr);
    return read_nametable(index);
}

void Mapper::do_write_nametable(uint16_t addr, uint8_t data)
{
    const uint16_t index = nametable_index(addr);
    write_nametable(index, data);
}

uint16_t Mapper::nametable_index(uint16_t addr) const
{
    const uint16_t index = addr - 0x2000;

    // vertical mirroring
    if (GetMirroring() == Mirroring::VERTICAL)
        return (index & 0x07FF) + 0x2000;

    // horizontal mirroring
    if (index >= 0x0000 && index <= 0x07FF)
        return (index & 0x03FF) + 0x2000;

    if (index >= 0x0800 && index <= 0x0FFF)
        return (0x400 | (index & 0x03FF)) + 0x2000;

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

    case 19:
        m = new Mapper_019(prg_data, chr_data);
        break;

    default:
        break;
    }

    return std::shared_ptr<Mapper>(m);
}

} // namespace
