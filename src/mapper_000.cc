#include "mapper_000.h"

namespace nes {

Mapper_000::Mapper_000(const std::vector<uint8_t> &prog_rom,
        const std::vector<uint8_t> &char_rom) : Mapper(prog_rom, char_rom)
{
    // PRG ROM size: 16 KiB for NROM-128, 32 KiB for NROM-256 (DIP-28 standard pinout)
    // PRG ROM bank size: Not bankswitched
    // PRG RAM: 2 or 4 KiB, not bankswitched, only in Family Basic
    // (but most emulators provide 8)
    if (GetProgRomSize() == 32 * 1024)
        mirroring_mask_ = 0x7FFF;
    else
        mirroring_mask_ = 0x3FFF;
}

Mapper_000::~Mapper_000()
{
}

uint8_t Mapper_000::do_read_prog(uint16_t addr) const
{

    // CPU $6000-$7FFF: Family Basic only: PRG RAM, mirrored as necessary to
    // fill entire 8 KiB window, write protectable with an external switch
    // CPU $8000-$BFFF: First 16 KB of ROM.
    // CPU $C000-$FFFF: Last 16 KB of ROM (NROM-256) or mirror of $8000-$BFFF (NROM-128).
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        const uint16_t a = addr & mirroring_mask_;
        return read_prog_rom(a);
    }
    else {
        return 0xFF;
    }
}

void Mapper_000::do_write_prog(uint16_t addr, uint8_t data)
{
}

uint8_t Mapper_000::do_read_char(uint16_t addr) const
{
    // CHR capacity: 8 KiB ROM (DIP-28 standard pinout) but most emulators support RAM
    // CHR bank size: Not bankswitched, see CNROM
    if (addr >= 0x0000 && addr <= 0x1FFF)
        return read_char_rom(addr);
    else
        return 0xFF;
}

void Mapper_000::do_write_char(uint16_t addr, uint8_t data)
{
}

} // namespace
