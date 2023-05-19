#include "mapper_002.h"

namespace nes {

Mapper_002::Mapper_002(const std::vector<uint8_t> &prog_rom,
        const std::vector<uint8_t> &char_rom) : Mapper(prog_rom, char_rom)
{
    // CPU $8000-$BFFF: 16 KB switchable PRG ROM bank
    // CPU $C000-$FFFF: 16 KB PRG ROM bank, fixed to the last bank
    const int prog_bank_count = GetProgRomSize() / 0x4000; // 16KB
    prog_bank_ = 0;
    prog_fixed_ = prog_bank_count - 1;

    use_char_ram(0x2000); // 8KB
}

Mapper_002::~Mapper_002()
{
}

uint8_t Mapper_002::do_read_prog(uint16_t addr) const
{
    if (addr >= 0x8000 && addr <= 0xBFFF) {
        const uint32_t a = prog_bank_ * 0x4000 + (addr & 0x3FFF);
        return read_prog_rom(a);
    }
    else if (addr >= 0xC000 && addr <= 0xFFFF) {
        const uint32_t a = prog_fixed_ * 0x4000 + (addr & 0x3FFF);
        return read_prog_rom(a);
    }
    else {
        return 0x00;
    }
}

void Mapper_002::do_write_prog(uint16_t addr, uint8_t data)
{
    // Bank select ($8000-$FFFF)
    // 7  bit  0
    // ---- ----
    // xxxx pPPP
    //      ||||
    //      ++++- Select 16 KB PRG ROM bank for CPU $8000-$BFFF
    //           (UNROM uses bits 2-0; UOROM uses bits 3-0)
    if (addr >= 0x8000 && addr <= 0xFFFF)
        prog_bank_ = data & 0x0F;
}

uint8_t Mapper_002::do_read_char(uint16_t addr) const
{
    if (addr >= 0x0000 && addr <= 0x1FFF)
        return read_char_ram(addr);
    else
        return 0xFF;
}

void Mapper_002::do_write_char(uint16_t addr, uint8_t data)
{
    if (addr >= 0x0000 && addr <= 0x1FFF)
        write_char_ram(addr, data);
}

} // namespace
