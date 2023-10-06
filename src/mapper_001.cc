#include "mapper_001.h"
#include <iostream>
#include <array>

namespace nes {

enum ChrBankMode {
    CHR_8KB = 0,
    CHR_4KB
};

enum PrgBankMode {
    PRG_32KB_0 = 0,
    PRG_32KB_1,
    PRG_FIX_FIRST,
    PRG_FIX_LAST
};

Mapper_001::Mapper_001(const std::vector<uint8_t> &prg_rom,
        const std::vector<uint8_t> &chr_rom) : Mapper(prg_rom, chr_rom)
{
    prg_.resize(GetPrgRomSize());
    chr_.resize(GetChrRomSize());

    shift_register_ = 0x10;
    prg_mode_ = PRG_FIX_LAST;
    chr_mode_ = CHR_8KB;
    prg_ram_disabled_ = false;

    if (GetChrRomSize() == 0)
        use_chr_ram(0x2000);

    use_prg_ram(0x2000);

    select_board();
    write_prg_bank(0x00);
    write_chr_bank(0, 0x00);
}

Mapper_001::~Mapper_001()
{
}

uint8_t Mapper_001::do_read_prg(uint16_t addr) const
{
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        const int index = addr - 0x6000;
        return read_prg_ram(index);
    }
    else if (addr >= 0x8000 && addr <= 0xFFFF) {
        const int index = prg_.map(addr - 0x8000);
        return read_prg_rom(index);
    }
    else {
        return 0;
    }
}

uint8_t Mapper_001::do_read_chr(uint16_t addr) const
{
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        if (is_chr_ram_used()) {
            return read_chr_ram(addr);
        }
        else {
            const int index = chr_.map(addr);
            return read_chr_rom(index);
        }
    }
    else {
        return 0xFF;
    }
}

void Mapper_001::do_write_prg(uint16_t addr, uint8_t data)
{
    // Shift register
    // To switch a bank, a program will execute code similar to the following:
    //
    // ;
    // ; Sets the switchable PRG ROM bank to the value of A.
    // ;
    //               ;  A          MMC1_SR  MMC1_PB
    // setPRGBank:   ;  000edcba    10000             Start with an empty shift register (SR).
    //   sta $E000   ;  000edcba -> a1000             The 1 is used to detect when the SR has
    //   lsr a       ; >0000edcb    a1000             become full.
    //   sta $E000   ;  0000edcb -> ba100
    //   lsr a       ; >00000edc    ba100
    //   sta $E000   ;  00000edc -> cba10
    //   lsr a       ; >000000ed    cba10
    //   sta $E000   ;  000000ed -> dcba1             Once a 1 is shifted into the last
    //   lsr a       ; >0000000e    dcba1             position, the SR is full.
    //   sta $E000   ;  0000000e    dcba1 -> edcba    A write with the SR full copies D0 and
    //
    //               ;              10000             the SR to a bank register ($E000-$FFFF
    //   rts                                          means PRG bank number) and then clears
    //                                                the SR.
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        if (prg_ram_disabled_)
            return;
        const int index = addr - 0x6000;
        write_prg_ram(index, data);
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
            prg_mode_ = PRG_FIX_LAST;
            control_register_ |= 0x0C;
        } else {
            const bool fifth_write = shift_register_ & 0x01;

            shift_register_ >>= 1;
            shift_register_ |= (data & 0x01) << 4;

            if (fifth_write) {
                if (addr >= 0x8000 && addr <= 0x9FFF)
                    write_control(shift_register_);
                else if (addr >= 0xA000 && addr <= 0xBFFF)
                    write_chr_bank(0, shift_register_);
                else if (addr >= 0xC000 && addr <= 0xDFFF)
                    write_chr_bank(1, shift_register_);
                else if (addr >= 0xE000 && addr <= 0xFFFF)
                    write_prg_bank(shift_register_);

                shift_register_ = 0x10;
            }
        }
    }
}

void Mapper_001::do_write_chr(uint16_t addr, uint8_t data)
{
    if (!is_chr_ram_used())
        return;

    if (addr >= 0x0000 && addr <= 0x1FFF)
        write_chr_ram(addr, data);
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

    const int mirroring = data & 0x03;
    prg_mode_ = (data >> 2) & 0x03;
    chr_mode_ = (data >> 4) & 0x01;

    if (mirroring == 3)
        SetMirroring(Mirroring::HORIZONTAL);
    else if (mirroring == 2)
        SetMirroring(Mirroring::VERTICAL);
}

void Mapper_001::write_prg_bank(uint8_t data)
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
    switch (prg_mode_) {
    case PRG_32KB_0:
    case PRG_32KB_1:
        prg_.select(0, (data & 0x0E));
        prg_.select(1, (data & 0x0E) + 1);
        break;

    case PRG_FIX_FIRST:
        prg_.select(0, 0);
        prg_.select(1, data & 0x0F);
        break;

    case PRG_FIX_LAST:
        prg_.select(0, data & 0x0F);
        prg_.select(1, -1);
        break;

    default:
        break;
    }
    // TODO PRG RAM chip enable
}

enum MMC1_Board {
    SxROM = 0,
    SLROM,
    SNROM,
    SUROM,
};

static bool match_size(const std::array<uint16_t,4> &size_list, uint16_t key_size)
{
    int list_count = size_list.size();

    // check i from 3 to 1 if size if zero.
    for (int i = list_count - 1; i > 0; i--) {
        if (size_list[i] != 0)
            break;
        list_count--;
    }

    for (int i = 0; i < list_count; i++) {
        if (size_list[i] == key_size)
            return true;
    }

    return false;
}

static int find_board(uint32_t PRG_ROM, uint32_t PRG_RAM, uint32_t CHR, std::string &name)
{
    static const struct MMC1_Board_Entry {
        const char *name = nullptr;
        std::array<uint16_t,4> prg_rom;
        std::array<uint16_t,4> prg_ram;
        std::array<uint16_t,4> chr;
        int board;
    } board_entries[] = {
        // Board   PRG ROM      PRG RAM   CHR
        {"SLROM",  {128, 256},  {0},      {128} , SLROM },
        {"SNROM",  {128, 256},  {8},      {8}   , SNROM },
        {"SUROM",  {512},       {8},      {8}   , SUROM },
    };

    for (const auto &entry: board_entries) {
        const bool found =
            match_size(entry.prg_rom, PRG_ROM) &&
            match_size(entry.prg_ram, PRG_RAM) &&
            match_size(entry.chr, CHR);

        if (found) {
            name = entry.name;
            return entry.board;
        }
    }

    name = "SxROM";
    return SxROM;
}

void Mapper_001::select_board()
{
    const uint32_t PRG_ROM = GetPrgRomSize() / 1024;
    const uint32_t PRG_RAM = GetPrgRamSize() / 1024;
    const uint32_t CHR = (is_chr_ram_used() ? GetChrRamSize() : GetChrRomSize()) / 1024;
    std::string name;

    mmc3_board_ = find_board(PRG_ROM, PRG_RAM, CHR, name);
    set_board_name(name);
}

void Mapper_001::write_chr_bank(int window, uint8_t data)
{
    switch (mmc3_board_) {

    case SNROM:
        if (window == 0) {
            // CHR bank 0 (internal, $A000-$BFFF)
            // 4bit0
            // -----
            // ExxxC
            // |   |
            // |   +- Select 4 KB CHR RAM bank at PPU $0000 (ignored in 8 KB mode)
            // +----- PRG RAM disable (0: enable, 1: open bus)
            if (chr_mode_ == CHR_4KB) {
                chr_.select(0, data & 0x01);
            }
            prg_ram_disabled_ = (data & 0x10);
        }
        else if (window == 1) {
            // CHR bank 1 (internal, $C000-$DFFF)
            // 4bit0
            // -----
            // ExxxC
            // |   |
            // |   +- Select 4 KB CHR RAM bank at PPU $1000 (ignored in 8 KB mode)
            // +----- PRG RAM disable (0: enable, 1: open bus) (ignored in 8 KB mode)
            if (chr_mode_ == CHR_4KB) {
                chr_.select(1, data & 0x01);
                prg_ram_disabled_ = (data & 0x10);
            }
        }
        break;

    case SUROM:
        if (window == 0) {
            // CHR bank 0 (internal, $A000-$BFFF)
            // 4bit0
            // -----
            // PSSxC
            // ||| |
            // ||| +- Select 4 KB CHR RAM bank at PPU $0000 (ignored in 8 KB mode)
            // |++--- Select 8 KB PRG RAM bank
            // +----- Select 256 KB PRG ROM bank
            if (chr_mode_ == CHR_4KB) {
                chr_.select(0, data & 0x01);
            }
            prg_ram_bank_ = (data >> 2) & 0x03;
            prg_.select(0, (prg_.bank(0) & 0x0F) | (data & 0x10));
            prg_.select(1, (prg_.bank(1) & 0x0F) | (data & 0x10));
        }
        else if (window == 1) {
            // CHR bank 1 (internal, $C000-$DFFF)
            // 4bit0
            // -----
            // PSSxC
            // ||| |
            // ||| +- Select 4 KB CHR RAM bank at PPU $1000 (ignored in 8 KB mode)
            // |++--- Select 8 KB PRG RAM bank (ignored in 8 KB mode)
            // +----- Select 256 KB PRG ROM bank (ignored in 8 KB mode)
            if (chr_mode_ == CHR_4KB) {
                chr_.select(1, data & 0x01);
                prg_ram_bank_ = (data >> 2) & 0x03;
                prg_.select(0, (prg_.bank(0) & 0x0F) | (data & 0x10));
                prg_.select(1, (prg_.bank(1) & 0x0F) | (data & 0x10));
            }
        }
        break;

    case SLROM:
    case SxROM:
    default:
        if (window == 0) {
            // CHR bank 0 (internal, $A000-$BFFF)
            // 4bit0
            // -----
            // CCCCC
            // |||||
            // +++++- Select 4 KB or 8 KB CHR bank at PPU $0000
            //        (low bit ignored in 8 KB mode)
            if (chr_mode_ == CHR_4KB) {
                chr_.select(0, data & 0x1F);
            }
            else {
                chr_.select(0, (data & 0x1E));
                chr_.select(1, (data & 0x1E) + 1);
            }
        }
        else if (window == 1) {
            // CHR bank 1 (internal, $C000-$DFFF)
            // 4bit0
            // -----
            // CCCCC
            // |||||
            // +++++- Select 4 KB CHR bank at PPU $1000 (ignored in 8 KB mode)
            if (chr_mode_ == CHR_4KB) {
                chr_.select(1, data & 0x1F);
            }
        };
        break;
    }
}

} // namespace
