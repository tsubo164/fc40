#include "mapper_002.h"

namespace nes {

Mapper_002::Mapper_002(const std::vector<uint8_t> &prg_rom,
        const std::vector<uint8_t> &chr_rom) : Mapper(prg_rom, chr_rom)
{
    // CPU $8000-$BFFF: 16 KB switchable PRG ROM bank
    // CPU $C000-$FFFF: 16 KB PRG ROM bank, fixed to the last bank
    prg_.resize(GetPrgRomSize());
    prg_.select(1, -1);

    use_chr_ram(0x2000); // 8KB
}

Mapper_002::~Mapper_002()
{
}

uint8_t Mapper_002::do_read_prg(uint16_t addr) const
{
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        const int index = prg_.map(addr - 0x8000);
        return read_prg_rom(index);
    }
    else {
        return 0x00;
    }
}

uint8_t Mapper_002::do_read_chr(uint16_t addr) const
{
    if (addr >= 0x0000 && addr <= 0x1FFF)
        return read_chr_ram(addr);
    else
        return 0xFF;
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
        prg_.select(0, data & 0x0F);
}

void Mapper_002::do_write_chr(uint16_t addr, uint8_t data)
{
    if (addr >= 0x0000 && addr <= 0x1FFF)
        write_chr_ram(addr, data);
}

void Mapper_002::do_get_prg_bank_info(BankInfo &info) const
{
    GetBankInfo(prg_, info);
}

void Mapper_002::do_get_chr_bank_info(BankInfo &info) const
{
    GetDefaultBankInfo(info);
}

} // namespace
