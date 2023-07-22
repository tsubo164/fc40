#include "mapper_002.h"

namespace nes {

Mapper_002::Mapper_002(const std::vector<uint8_t> &prg_rom,
        const std::vector<uint8_t> &chr_rom) : Mapper(prg_rom, chr_rom)
{
    // CPU $8000-$BFFF: 16 KB switchable PRG ROM bank
    // CPU $C000-$FFFF: 16 KB PRG ROM bank, fixed to the last bank
    const int prg_bank_count = GetPrgRomSize() / 0x4000; // 16KB
    prg_bank_ = 0;
    prg_fixed_ = prg_bank_count - 1;

    use_chr_ram(0x2000); // 8KB
}

Mapper_002::~Mapper_002()
{
}

uint8_t Mapper_002::do_read_prg(uint16_t addr) const
{
    if (addr >= 0x8000 && addr <= 0xBFFF) {
        const uint32_t a = prg_bank_ * 0x4000 + (addr & 0x3FFF);
        return read_prg_rom(a);
    }
    else if (addr >= 0xC000 && addr <= 0xFFFF) {
        const uint32_t a = prg_fixed_ * 0x4000 + (addr & 0x3FFF);
        return read_prg_rom(a);
    }
    else {
        return 0x00;
    }
}

void Mapper_002::do_write_prg(uint16_t addr, uint8_t data)
{
    // Bank select ($8000-$FFFF)
    // 7  bit  0
    // ---- ----
    // xxxx pPPP
    //      ||||
    //      ++++- Select 16 KB PRG ROM bank for CPU $8000-$BFFF
    //           (UNROM uses bits 2-0; UOROM uses bits 3-0)
    if (addr >= 0x8000 && addr <= 0xFFFF)
        prg_bank_ = data & 0x0F;
}

uint8_t Mapper_002::do_read_chr(uint16_t addr) const
{
    if (addr >= 0x0000 && addr <= 0x1FFF)
        return read_chr_ram(addr);
    else
        return 0xFF;
}

void Mapper_002::do_write_chr(uint16_t addr, uint8_t data)
{
    if (addr >= 0x0000 && addr <= 0x1FFF)
        write_chr_ram(addr, data);
}

} // namespace
