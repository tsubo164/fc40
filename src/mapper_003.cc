#include "mapper_003.h"

namespace nes {

Mapper_003::Mapper_003(const std::vector<uint8_t> &prog_rom,
        const std::vector<uint8_t> &char_rom) : Mapper(prog_rom, char_rom)
{
    // PRG ROM size: 16 KiB or 32 KiB
    // PRG ROM bank size: Not bankswitched
    // PRG RAM: None
    // CHR capacity: Up to 2048 KiB ROM
    // CHR bank size: 8 KiB
    //prog_nbanks_ = GetProgRomSize() / 0x4000; // 16KB
    if (GetProgRomSize() == 32 * 1024)
        prog_mirroring_mask_ = 0x7FFF;
    else
        prog_mirroring_mask_ = 0x3FFF;
    char_bank_ = 0;
}

Mapper_003::~Mapper_003()
{
}

uint8_t Mapper_003::do_read_prog(uint16_t addr) const
{
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        const uint16_t a = addr & prog_mirroring_mask_;
        return read_prog_rom(a);
    }
    return 0;
}

void Mapper_003::do_write_prog(uint16_t addr, uint8_t data)
{
    // 7  bit  0
    // ---- ----
    // cccc ccCC
    // |||| ||||
    // ++++-++++- Select 8 KB CHR ROM bank for PPU $0000-$1FFF
    // CNROM only implements the lowest 2 bits, capping it at 32 KiB CHR.
    // Other boards may implement 4 or more bits for larger CHR.
    if (addr >= 0x8000 && addr <= 0xFFFF)
        char_bank_ = data & 0x03;
}

uint8_t Mapper_003::do_read_char(uint16_t addr) const
{
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        const uint32_t a = char_bank_ * 0x2000 + addr;
        return read_char_rom(a);
    } else {
        return 0xFF;
    }
}

void Mapper_003::do_write_char(uint16_t addr, uint8_t data)
{
}

} // namespace
