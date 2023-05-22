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

Mapper_001::Mapper_001(const std::vector<uint8_t> &prog_rom,
        const std::vector<uint8_t> &char_rom) : Mapper(prog_rom, char_rom)
{
    prog_nbanks_ = GetProgRomSize() / 0x4000; // 16KB
    char_nbanks_ = GetCharRomSize() / 0x2000; // 8KB

    prog_bank_0_ = 0;
    prog_bank_1_ = 1;
    char_bank_0_ = 0;
    char_bank_1_ = 1;

    shift_counter_ = 0;
    shift_register_ = 0;
    prog_bank_mode_ = PROG_BANK_FIX_LAST;
    char_bank_mode_ = 0;

    if (GetCharRomSize() == 0) {
        use_char_ram(0x2000);
        use_char_ram_ = true;
    }
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
            shift_register_ = 0;
            prog_bank_mode_ = PROG_BANK_FIX_LAST;

            control_register_ |= 0x0C;
        } else {
            shift_register_ >>= 1;
            shift_register_ |= (data & 0x01) << 4;
            shift_counter_++;

            if (shift_counter_ == 5) {
                if (addr >= 0x8000 && addr <= 0x9FFF)
                    write_control(shift_register_);
                else if (addr >= 0xA000 && addr <= 0xBFFF)
                    write_char_bank_0(shift_register_);
                else if (addr >= 0xC000 && addr <= 0xDFFF)
                    write_char_bank_1(shift_register_);
                else if (addr >= 0xE000 && addr <= 0xFFFF)
                    write_prog_bank(shift_register_);

                shift_register_ = 0;
                shift_counter_ = 0;
            }
        }
    }
}

uint8_t Mapper_001::do_read_char(uint16_t addr) const
{
    if (addr >= 0x0000 && addr <= 0x0FFF) {
        const uint32_t a = char_bank_0_ * 0x1000 + (addr & 0x0FFF);
        if (use_char_ram_)
            return read_char_ram(a);
        else
            return read_char_rom(a);
    }
    else if (addr >= 0x1000 && addr <= 0x1FFF) {
        const uint32_t a = char_bank_1_ * 0x1000 + (addr & 0x0FFF);
        if (use_char_ram_)
            return read_char_ram(a);
        else
            return read_char_rom(a);
    }
    else {
        return 0xFF;
    }
}

void Mapper_001::do_write_char(uint16_t addr, uint8_t data)
{
    if (!use_char_ram_)
        return;

    if (addr >= 0x0000 && addr <= 0x0FFF) {
        const uint32_t a = char_bank_0_ * 0x1000 + (addr & 0x0FFF);
        write_char_ram(a, data);
    }
    else if (addr >= 0x1000 && addr <= 0x1FFF) {
        const uint32_t a = char_bank_1_ * 0x1000 + (addr & 0x0FFF);
        write_char_ram(a, data);
    }
}

void Mapper_001::write_control(uint8_t data)
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
    control_register_ = data;

    mirror_ = data & 0x03;
    prog_bank_mode_ = (data >> 2) & 0x03;
    char_bank_mode_ = (data >> 4) & 0x01;
}

void Mapper_001::write_char_bank_0(uint8_t data)
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
        char_bank_0_ = (data & 0x1F);
        break;

    case CHAR_BANK_8KB:
        char_bank_0_ = (data & 0x1E); // 0, 2, 4, ...
        char_bank_1_ = char_bank_0_ + 1;         // 1, 3, 5, ...
        break;

    default:
        break;
    }
}

void Mapper_001::write_char_bank_1(uint8_t data)
{
    // CHR bank 1 (internal, $C000-$DFFF)
    // 4bit0
    // -----
    // CCCCC
    // |||||
    // +++++- Select 4 KB CHR bank at PPU $1000 (ignored in 8 KB mode)
    switch (char_bank_mode_) {
    case CHAR_BANK_4KB:
        char_bank_1_ = (data & 0x1F);
        break;

    case CHAR_BANK_8KB:
        break;

    default:
        break;
    }
}

void Mapper_001::write_prog_bank(uint8_t data)
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
        prog_bank_0_ = (data & 0x0E); // 0, 2, 4, ...
        prog_bank_1_ = prog_bank_0_ + 1;         // 1, 3, 5, ...
        break;

    case PROG_BANK_FIX_FIRST:
        prog_bank_0_ = 0;
        prog_bank_1_ = (data & 0x0F);
        break;

    case PROG_BANK_FIX_LAST:
        prog_bank_0_ = (data & 0x0F);
        prog_bank_1_ = prog_nbanks_ - 1;
        break;

    default:
        break;
    }
}

} // namespace
