#include "mapper_019.h"
#include <iostream>

namespace nes {

Mapper_019::Mapper_019(const std::vector<uint8_t> &prg_rom,
        const std::vector<uint8_t> &chr_rom) : Mapper(prg_rom, chr_rom)
{
    prg_.resize(GetPrgRomSize());
    chr_.resize(GetChrRomSize());

    prg_.select(3, -1);
    use_prg_ram(0x2000);
}

Mapper_019::~Mapper_019()
{
}

uint8_t Mapper_019::do_read_prg(uint16_t addr) const
{
    if (addr >= 0x4800 && addr <= 0x4FFF) {
        // Chip RAM Data Port ($4800-$4FFF) r/w
    }
    else if (addr >= 0x5000 && addr <= 0x57FF) {
        // IRQ Counter (low) ($5000-$57FF) r/w
        // 7  bit  0
        // ---- ----
        // IIII IIII
        // |||| ||||
        // ++++-++++- Low 8 bits of IRQ counter
        return irq_counter_ & 0x00FF;
    }
    else if (addr >= 0x5800 && addr <= 0x5FFF) {
        // IRQ Counter (high) / IRQ Enable ($5800-$5FFF) r/w
        // 7  bit  0
        // ---- ----
        // EIII IIII
        // |||| ||||
        // |+++-++++- High 7 bits of IRQ counter
        // +--------- IRQ Enable: (0: disabled; 1: enabled)
        return ((irq_counter_ >> 8) & 0x007F) | (irq_enabled_ << 7);
    }
    else if (addr >= 0x6000 && addr <= 0x7FFF) {
        const int index = prg_.map(addr - 0x6000);
        return read_prg_ram(index);
    }
    else if (addr >= 0x8000 && addr <= 0xFFFF) {
        const int index = prg_.map(addr - 0x8000);
        return read_prg_rom(index);
    }

    return 0x00;
}

void Mapper_019::do_write_prg(uint16_t addr, uint8_t data)
{
    if (addr >= 0x4800 && addr <= 0x4FFF) {
        // Chip RAM Data Port ($4800-$4FFF) r/w
    }
    else if (addr >= 0x5000 && addr <= 0x57FF) {
        // IRQ Counter (low) ($5000-$57FF) r/w
        // 7  bit  0
        // ---- ----
        // IIII IIII
        // |||| ||||
        // ++++-++++- Low 8 bits of IRQ counter
        irq_counter_ = (irq_counter_ & 0xFF00) | data;
        ClearIRQ();
    }
    else if (addr >= 0x5800 && addr <= 0x5FFF) {
        // IRQ Counter (high) / IRQ Enable ($5800-$5FFF) r/w
        // 7  bit  0
        // ---- ----
        // EIII IIII
        // |||| ||||
        // |+++-++++- High 7 bits of IRQ counter
        // +--------- IRQ Enable: (0: disabled; 1: enabled)
        irq_counter_ = (irq_counter_ & 0x00FF) | ((data & 0x7F) << 8);
        irq_enabled_ = data & 0x80;
        ClearIRQ();
    }
    else if (addr >= 0x6000 && addr <= 0x7FFF) {
        const int index = prg_.map(addr - 0x6000);
        write_prg_ram(index, data);
    }
    else if (addr >= 0x8000 && addr <= 0xDFFF) {
        // CHR and NT Select ($8000-$DFFF) w
        // -----------------------------------------------
        // Value CPU   | Behavior
        // writes      |
        // -----------------------------------------------
        // $00-$DF     | Always selects 1KB page of CHR-ROM
        // $E0-$FF     | If enabled by bit in $E800, use the NES's internal
        //             | nametables (even values for A, odd values for B)
        // -----------------------------------------------
        // Write to    | 1KB CHRbank | Values >= $E0
        // CPU address | affected    | denote NES NTRAM if
        // -----------------------------------------------
        // $8000-$87FF | $0000-$03FF | $E800.6 = 0
        // $8800-$8FFF | $0400-$07FF | $E800.6 = 0
        // $9000-$97FF | $0800-$0BFF | $E800.6 = 0
        // $9800-$9FFF | $0C00-$0FFF | $E800.6 = 0
        // $A000-$A7FF | $1000-$13FF | $E800.7 = 0
        // $A800-$AFFF | $1400-$17FF | $E800.7 = 0
        // $B000-$B7FF | $1800-$1BFF | $E800.7 = 0
        // $B800-$BFFF | $1C00-$1FFF | $E800.7 = 0
        // $C000-$C7FF | $2000-$23FF | always
        // $C800-$CFFF | $2400-$27FF | always
        // $D000-$D7FF | $2800-$2BFF | always
        // $D800-$DFFF | $2C00-$2FFF | always
        const int window = (addr - 0x8000) / 0x0800;
        const bool ntram_select_table[] = {
            use_ntram_lo_, use_ntram_lo_, use_ntram_lo_, use_ntram_lo_,
            use_ntram_hi_, use_ntram_hi_, use_ntram_hi_, use_ntram_hi_,
            true,          true,          true,          true
        };
        const bool denote_ntram = data >= 0xE0 && ntram_select_table[window];

        if (denote_ntram) {
            bank_select_[window] = (data & 0x01) ? SELECT_NTRAM_HI : SELECT_NTRAM_LO;
        }
        else {
            bank_select_[window] = SELECT_CHR_ROM;
            chr_.select(window, data);
        }
    }
    else if (addr >= 0xE000 && addr <= 0xE7FF) {
        // PRG Select 1 ($E000-$E7FF) w
        // 7  bit  0
        // ---- ----
        // AMPP PPPP
        // |||| ||||
        // ||++-++++- Select 8KB page of PRG-ROM at $8000
        // |+-------- Disable sound if set
        // +--------- Pin 22 (open collector) reflects the inverse of
        //            this value, unchanged by the address bus inputs.
        prg_.select(0, data & 0x3F);
    }
    else if (addr >= 0xE800 && addr <= 0xEFFF) {
        // PRG Select 2 / CHR-RAM Enable ($E800-$EFFF) w
        // 7  bit  0
        // ---- ----
        // HLPP PPPP
        // |||| ||||
        // ||++-++++- Select 8KB page of PRG-ROM at $A000
        // |+-------- Disable CHR-RAM at $0000-$0FFF
        // |            0: Pages $E0-$FF use NT RAM as CHR-RAM
        // |            1: Pages $E0-$FF are the last $20 banks of CHR-ROM
        // +--------- Disable CHR-RAM at $1000-$1FFF
        //              0: Pages $E0-$FF use NT RAM as CHR-RAM
        //              1: Pages $E0-$FF are the last $20 banks of CHR-ROM
        prg_.select(1, data & 0x3F);
        use_ntram_hi_ = !((data >> 6) & 0x01);
        use_ntram_lo_ = !((data >> 7) & 0x01);
    }
    else if (addr >= 0xF000 && addr <= 0xF7FF) {
        // PRG Select 3 ($F000-$F7FF) w
        // 7  bit  0
        // ---- ----
        // B.PP PPPP
        // | || ||||
        // | ++-++++- Select 8KB page of PRG-ROM at $C000
        // +--------- Pin 44 reflects this value.
        prg_.select(2, data & 0x3F);
    }
    else if (addr >= 0xF800 && addr <= 0xFFFF) {
        // Write Protect for External RAM AND Chip RAM Address Port
        // ( $F800 - $FFFF ) w
        // 7  bit  0
        // ---- ----
        // KKKK DCBA
        // |||| ||||
        // |||| |||+- 1: Write - protect 2kB window of external RAM from
        // |||| |||      $6000 - $67FF( 0: write enable )
        // |||| ||+-- 1: Write - protect 2kB window of external RAM from
        // |||| ||       $6800 - $6FFF( 0: write enable )
        // |||| |+--- 1: Write - protect 2kB window of external RAM from
        // |||| |        $7000 - $77FF( 0: write enable )
        // |||| +---- 1: Write - protect 2kB window of external RAM from
        // ||||          $7800 - $7FFF( 0: write enable )
        // ++++------Additionally the upper nybble must be equal to b0100
        //           to enable writes
    }
}

uint8_t Mapper_019::read_chr(uint16_t addr) const
{
    const int window = addr / 0x400;
    switch (bank_select_[window]) {

    case SELECT_CHR_ROM:
        {
            const int index = chr_.map(addr);
            return read_chr_rom(index);
        }

    case SELECT_NTRAM_LO:
        {
            const int index = (addr & 0x3FF);
            return read_nametable(index);
        }

    case SELECT_NTRAM_HI:
        {
            const int index = (addr & 0x3FF) + 0x400;
            return read_nametable(index);
        }

    default:
        return 0xFF;
    }
}

void Mapper_019::write_chr(uint16_t addr, uint8_t data)
{
    const int window = addr / 0x400;
    switch (bank_select_[window]) {

    case SELECT_CHR_ROM:
        break;

    case SELECT_NTRAM_LO:
        {
            const int index = (addr & 0x3FF);
            write_nametable(index, data);
        }
        break;

    case SELECT_NTRAM_HI:
        {
            const int index = (addr & 0x3FF) + 0x400;
            write_nametable(index, data);
        }
        break;

    default:
        break;
    }
}

uint8_t Mapper_019::do_read_chr(uint16_t addr) const
{
    if (addr >= 0x0000 && addr <= 0x1FFF)
        return read_chr(addr);
    else
        return 0xFF;
}

uint8_t Mapper_019::do_read_nametable(uint16_t addr) const
{
    if (addr >= 0x2000 && addr <= 0x2FFF)
        return read_chr(addr);
    else
        return 0xFF;
}

void Mapper_019::do_write_chr(uint16_t addr, uint8_t data)
{
    if (addr >= 0x0000 && addr <= 0x1FFF)
        write_chr(addr, data);
}

void Mapper_019::do_write_nametable(uint16_t addr, uint8_t data)
{
    if (addr >= 0x2000 && addr <= 0x2FFF)
        write_chr(addr, data);
}

void Mapper_019::do_cpu_clock()
{
    if (!irq_enabled_)
        return;

    if (irq_counter_ == 0x7FFF)
        set_irq();
    else
        irq_counter_++;
}

} // namespace
