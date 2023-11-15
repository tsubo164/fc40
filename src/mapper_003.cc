#include "mapper_003.h"

namespace nes {

Mapper_003::Mapper_003(const std::vector<uint8_t> &prg_rom,
        const std::vector<uint8_t> &chr_rom) : Mapper(prg_rom, chr_rom)
{
    // PRG ROM size: 16 KiB or 32 KiB
    // PRG ROM bank size: Not bankswitched
    // PRG RAM: None
    // CHR capacity: Up to 2048 KiB ROM
    // CHR bank size: 8 KiB
    prg_.resize(GetPrgRomSize());
    chr_.resize(GetChrRomSize());
}

Mapper_003::~Mapper_003()
{
}

uint8_t Mapper_003::do_read_prg(uint16_t addr) const
{
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        const int index = prg_.map(addr - 0x8000);
        return read_prg_rom(index);
    }
    else {
        return 0x00;
    }
}

uint8_t Mapper_003::do_read_chr(uint16_t addr) const
{
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        const int index = chr_.map(addr);
        return read_chr_rom(index);
    }
    else {
        return 0xFF;
    }
}

void Mapper_003::do_write_prg(uint16_t addr, uint8_t data)
{
    // Bank select ($8000-$FFFF)
    // 7  bit  0
    // ---- ----
    // cccc ccCC
    // |||| ||||
    // ++++-++++- Select 8 KB CHR ROM bank for PPU $0000-$1FFF
    // CNROM only implements the lowest 2 bits, capping it at 32 KiB CHR.
    // Other boards may implement 4 or more bits for larger CHR.
    if (addr >= 0x8000 && addr <= 0xFFFF)
        chr_.select(0, data & 0x03);
}

void Mapper_003::do_write_chr(uint16_t addr, uint8_t data)
{
}

void Mapper_003::do_get_prg_bank_info(BankInfo &info) const
{
    GetBankInfo(prg_, info);
}

void Mapper_003::do_get_chr_bank_info(BankInfo &info) const
{
    GetBankInfo(chr_, info);
}

} // namespace
