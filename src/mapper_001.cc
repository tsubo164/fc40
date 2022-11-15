#include <cstdio>
#include "mapper_001.h"

namespace nes {

enum CharBankMode {
    CHAR_BANK_8KB = 0,
    CHAR_BANK_4KB
};

enum ProgBankMode {
    PROG_BANK_32KB_0 = 0,
    PROG_BANK_32KB_1,
    PROG_BANK_FIX_FIRST,
    PROG_BANK_FIX_LAST
};

Mapper_001::Mapper_001(const uint8_t *prog_rom, size_t prog_size,
        const uint8_t *char_rom, size_t char_size) :
    Mapper(prog_rom, prog_size, char_rom, char_size)
{
    prog_nbanks_ = prog_size / 0x4000; // 16KB
    char_nbanks_ = char_size / 0x2000; // 8KB

    prog_bank_0_ = 0;
    prog_bank_1_ = 1;
    char_bank_0_ = 0;
    char_bank_1_ = 1;

    shift_register_ = 0x10;
    prog_bank_mode_ = PROG_BANK_FIX_LAST;
    char_bank_mode_ = 0;
}

Mapper_001::~Mapper_001()
{
}

uint8_t Mapper_001::do_read_prog(uint16_t addr) const
{
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        return prog_ram_[addr & 0x1FFF];
    }
    else if (addr >= 0x8000 && addr <= 0xBFFF) {
        const uint32_t a = prog_bank_0_ * 0x4000 + (addr & 0x3FFF);
        return read_prog_rom(a);
    }
    else if (addr >= 0xC000 && addr <= 0xFFFF) {
        const uint32_t a = prog_bank_1_ * 0x4000 + (addr & 0x3FFF);
        return read_prog_rom(a);
    }
    else {
        return 0;
    }
}

void Mapper_001::do_write_prog(uint16_t addr, uint8_t data)
{
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        prog_ram_[addr & 0x1FFF] = data;
    }
    else if (addr >= 0x8000 && addr <= 0xFFFF) {
        // Load register ($8000-$FFFF)
        // 7  bit  0
        // ---- ----
        // Rxxx xxxD
        // |       |
        // |       +- Data bit to be shifted into shift register, LSB first
        // +--------- A write with bit set will reset shift register
        //            and write Control with (Control OR $0C),
        //            locking PRG ROM at $C000-$FFFF to the last bank.
        if (data & 0x80) {
            shift_register_ = 0x10;
            prog_bank_mode_ = PROG_BANK_FIX_LAST;
        } else {
            const bool fifth_write = shift_register_ & 0x01;

            shift_register_ >>= 1;
            shift_register_ |= (data & 0x01) << 4;

            if (fifth_write) {
                if (addr >= 0x8000 && addr <= 0x9FFF)
                    set_control();
                else if (addr >= 0xA000 && addr <= 0xBFFF)
                    set_char_bank_0();
                else if (addr >= 0xC000 && addr <= 0xDFFF)
                    set_char_bank_1();
                else if (addr >= 0xE000 && addr <= 0xFFFF)
                    set_prog_bank();

                shift_register_ = 0x10;
            }
        }
    }
}

uint8_t Mapper_001::do_read_char(uint16_t addr) const
{
    if (addr >= 0x0000 && addr <= 0x0FFF) {
        const uint32_t a = char_bank_0_ * 0x1000 + (addr & 0x0FFF);
        return read_char_rom(a);
    }
    else if (addr >= 0x1000 && addr <= 0x1FFF) {
        const uint32_t a = char_bank_1_ * 0x1000 + (addr & 0x0FFF);
        return read_char_rom(a);
    }
    else {
        return 0xFF;
    }
}

void Mapper_001::do_write_char(uint16_t addr, uint8_t data)
{
}

void Mapper_001::set_control()
{
    // Control (internal, $8000-$9FFF)
    // 4bit0
    // -----
    // CPPMM
    // |||||
    // |||++- Mirroring (0: one-screen, lower bank; 1: one-screen, upper bank;
    // |||               2: vertical; 3: horizontal)
    // |++--- PRG ROM bank mode
    // |      (0, 1: switch 32 KB at $8000, ignoring low bit of bank number;
    // |       2: fix first bank at $8000 and switch 16 KB bank at $C000;
    // |       3: fix last bank at $C000 and switch 16 KB bank at $8000)
    // +----- CHR ROM bank mode
    //        (0: switch 8 KB at a time; 1: switch two separate 4 KB banks)
    mirror_ = shift_register_ & 0x03;
    prog_bank_mode_ = (shift_register_ >> 2) & 0x03;
    char_bank_mode_ = (shift_register_ >> 4) & 0x01;
}

void Mapper_001::set_char_bank_0()
{
    // CHR bank 0 (internal, $A000-$BFFF)
    // 4bit0
    // -----
    // CCCCC
    // |||||
    // +++++- Select 4 KB or 8 KB CHR bank at PPU $0000
    //        (low bit ignored in 8 KB mode)
    switch (char_bank_mode_) {
    case CHAR_BANK_4KB:
        char_bank_0_ = (shift_register_ & 0x1F);
        break;

    case CHAR_BANK_8KB:
        char_bank_0_ = (shift_register_ & 0x1E); // 0, 2, 4, ...
        char_bank_1_ = char_bank_0_ + 1;         // 1, 3, 5, ...
        break;

    default:
        break;
    }
}

void Mapper_001::set_char_bank_1()
{
    // CHR bank 1 (internal, $C000-$DFFF)
    // 4bit0
    // -----
    // CCCCC
    // |||||
    // +++++- Select 4 KB CHR bank at PPU $1000 (ignored in 8 KB mode)
    switch (char_bank_mode_) {
    case CHAR_BANK_4KB:
        char_bank_1_ = (shift_register_ & 0x1F);
        break;

    case CHAR_BANK_8KB:
        break;

    default:
        break;
    }
}

void Mapper_001::set_prog_bank()
{
    // 4bit0
    // -----
    // RPPPP
    // |||||
    // |++++- Select 16 KB PRG ROM bank (low bit ignored in 32 KB mode)
    // +----- MMC1B and later: PRG RAM chip enable
    //        (0: enabled; 1: disabled; ignored on MMC1A)
    //        MMC1A: Bit 3 bypasses fixed bank logic in 16K mode
    //        (0: affected; 1: bypassed)
    switch (prog_bank_mode_) {
    case PROG_BANK_32KB_0:
    case PROG_BANK_32KB_1:
        prog_bank_0_ = (shift_register_ & 0x0E); // 0, 2, 4, ...
        prog_bank_1_ = prog_bank_0_ + 1;         // 1, 3, 5, ...
        break;

    case PROG_BANK_FIX_FIRST:
        prog_bank_0_ = 0;
        prog_bank_1_ = (shift_register_ & 0x0F);
        break;

    case PROG_BANK_FIX_LAST:
        prog_bank_0_ = (shift_register_ & 0x0F);
        prog_bank_1_ = prog_nbanks_ - 1;
        break;

    default:
        break;
    }
}

} // namespace
