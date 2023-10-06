#include "mapper_004.h"

namespace nes {

Mapper_004::Mapper_004(const std::vector<uint8_t> &prg_rom,
        const std::vector<uint8_t> &chr_rom) : Mapper(prg_rom, chr_rom)
{
    prg_.resize(GetPrgRomSize());
    chr_.resize(GetChrRomSize());
    prg_.select(2, -2);
    prg_.select(3, -1);

    if (GetChrRomSize() == 0) {
        use_chr_ram(0x2000);
        use_chr_ram_ = true;
    }

    use_prg_ram(0x2000);
}

Mapper_004::~Mapper_004()
{
}

uint8_t Mapper_004::do_read_prg(uint16_t addr) const
{
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        if (GetPrgRamSize() == 0)
            return 0xFF;
        const int index = addr - 0x6000;
        return read_prg_ram(index);
    }
    else if (addr >= 0x8000 && addr <= 0xFFFF) {
        const int index = prg_.map(addr - 0x8000);
        return read_prg_rom(index);
    }
    else {
        return 0xFF;
    }
}

uint8_t Mapper_004::do_read_chr(uint16_t addr) const
{
    if (use_chr_ram_) {
        const int index = addr & 0x1FFF;
        return read_chr_ram(index);
    }

    if (addr >= 0x0000 && addr <= 0x1FFF) {
        const int index = chr_.map(addr);
        return read_chr_rom(index);
    }
    else {
        return 0xFF;
    }
}

void Mapper_004::do_write_prg(uint16_t addr, uint8_t data)
{
    const bool even = addr % 2 == 0;

    if (addr >= 0x6000 && addr <= 0x7FFF) {
        if (!prg_ram_protected_)
            write_prg_ram(addr - 0x6000, data);
    }
    else if (addr >= 0x8000 && addr <= 0x9FFF) {
        if (even)
            set_bank_select(addr, data);
        else
            set_bank_data(addr, data);
    }
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (even)
            set_mirroring(data);
        else
            set_prg_ram_protect(data);
    }
    else if (addr >= 0xC000 && addr <= 0xDFFF) {
        if (even)
            set_irq_latch(data);
        else
            set_irq_reload(data);
    }
    else if (addr >= 0xE000 && addr <= 0xFFFF) {
        if (even)
            irq_disable(data);
        else
            irq_enable(data);
    }
}

void Mapper_004::do_write_chr(uint16_t addr, uint8_t data)
{
    if (!use_chr_ram_)
        return;

    const int index = addr & 0x1FFF;
    write_chr_ram(index, data);
}

void Mapper_004::set_bank_select(uint16_t addr, uint8_t data)
{
    // Bank select ($8000-$9FFE, even)
    // 7  bit  0
    // ---- ----
    // CPMx xRRR
    // |||   |||
    // |||   +++- Specify which bank register to update on next write to
    // |||        Bank Data register
    // |||          000: R0: Select 2 KB CHR bank at PPU $0000-$07FF (or $1000-$17FF)
    // |||          001: R1: Select 2 KB CHR bank at PPU $0800-$0FFF (or $1800-$1FFF)
    // |||          010: R2: Select 1 KB CHR bank at PPU $1000-$13FF (or $0000-$03FF)
    // |||          011: R3: Select 1 KB CHR bank at PPU $1400-$17FF (or $0400-$07FF)
    // |||          100: R4: Select 1 KB CHR bank at PPU $1800-$1BFF (or $0800-$0BFF)
    // |||          101: R5: Select 1 KB CHR bank at PPU $1C00-$1FFF (or $0C00-$0FFF)
    // |||          110: R6: Select 8 KB PRG ROM bank at $8000-$9FFF (or $C000-$DFFF)
    // |||          111: R7: Select 8 KB PRG ROM bank at $A000-$BFFF
    // ||+------- Nothing on the MMC3, see MMC6
    // |+-------- PRG ROM bank mode (0: $8000-$9FFF swappable,
    // |                                $C000-$DFFF fixed to second-last bank;
    // |                             1: $C000-$DFFF swappable,
    // |                                $8000-$9FFF fixed to second-last bank)
    // +--------- CHR A12 inversion (0: two 2 KB banks at $0000-$0FFF,
    //                                  four 1 KB banks at $1000-$1FFF;
    //                               1: two 2 KB banks at $1000-$1FFF,
    //                                  four 1 KB banks at $0000-$0FFF)
    bank_select_ = data & 0x07;
    prg_bank_mode_ = (data >> 6) & 0x01;
    chr_bank_mode_ = (data >> 7) & 0x01;
    // TODO CHR A12 inversion
}

void Mapper_004::set_bank_data(uint16_t addr, uint8_t data)
{
    // Bank data ($8001-$9FFF, odd)
    // 7  bit  0
    // ---- ----
    // DDDD DDDD
    // |||| ||||
    // ++++-++++- New bank value, based on last value written to
    //            Bank select register (mentioned above)
    if (chr_bank_mode_ == 0) {
        switch (bank_select_) {
        case 0:
            chr_.select(0, (data & 0xFE));
            chr_.select(1, (data & 0xFE) + 1);
            break;
        case 1:
            chr_.select(2, (data & 0xFE));
            chr_.select(3, (data & 0xFE) + 1);
            break;
        case 2:
            chr_.select(4, data);
            break;
        case 3:
            chr_.select(5, data);
            break;
        case 4:
            chr_.select(6, data);
            break;
        case 5:
            chr_.select(7, data);
            break;
        }
    }
    else {
        switch (bank_select_) {
        case 0:
            chr_.select(4, (data & 0xFE));
            chr_.select(5, (data & 0xFE) + 1);
            break;
        case 1:
            chr_.select(6, (data & 0xFE));
            chr_.select(7, (data & 0xFE) + 1);
            break;
        case 2:
            chr_.select(0, data);
            break;
        case 3:
            chr_.select(1, data);
            break;
        case 4:
            chr_.select(2, data);
            break;
        case 5:
            chr_.select(3, data);
            break;
        }
    }
    if (prg_bank_mode_ == 0) {
        switch (bank_select_) {
        case 6:
            prg_.select(0, data);
            prg_.select(2, -2);
            prg_.select(3, -1);
            break;
        case 7:
            prg_.select(1, data);
            prg_.select(2, -2);
            prg_.select(3, -1);
            break;
        }
    }
    else {
        switch (bank_select_) {
        case 6:
            prg_.select(2, data);
            prg_.select(0, -2);
            prg_.select(3, -1);
            break;
        case 7:
            prg_.select(1, data);
            prg_.select(0, -2);
            prg_.select(3, -1);
            break;
        }
    }
}

void Mapper_004::set_mirroring(uint8_t data)
{
    // Mirroring ($A000-$BFFE, even)
    // 7  bit  0
    // ---- ----
    // xxxx xxxM
    //         |
    //         +- Nametable mirroring (0: vertical; 1: horizontal)
    if ((data & 0x01) == 0)
        SetMirroring(Mirroring::VERTICAL);
    else
        SetMirroring(Mirroring::HORIZONTAL);
}

void Mapper_004::set_prg_ram_protect(uint8_t data)
{
    // 7  bit  0
    // ---- ----
    // RWXX xxxx
    // ||||
    // ||++------ Nothing on the MMC3, see MMC6
    // |+-------- Write protection (0: allow writes; 1: deny writes)
    // +--------- PRG RAM chip enable (0: disable; 1: enable)
    //
    // Disabling PRG RAM through bit 7 causes reads from the PRG RAM region to
    // return open bus. Though these bits are functional on the MMC3, their
    // main purpose is to write-protect save RAM during power-off. Many emulators
    // choose not to implement them as part of iNES Mapper 4 to avoid
    // an incompatibility with the MMC6.
    //
    // See iNES Mapper 004 and MMC6 below.
    prg_ram_protected_ = (data >> 6) & 0x01;
}

void Mapper_004::set_irq_latch(uint8_t data)
{
    // This register specifies the IRQ counter reload value.
    // When the IRQ counter is zero (or a reload is requested through $C001),
    // this value will be copied to the IRQ counter at the NEXT rising
    // edge of the PPU address, presumably at PPU cycle 260 of
    // the current scanline.
    irq_latch_ = data;
}

void Mapper_004::set_irq_reload(uint8_t data)
{
    // Writing any value to this register clears the MMC3 IRQ counter
    // immediately, and then reloads it at the NEXT rising edge of the
    // PPU address, presumably at PPU cycle 260 of the current scanline.
    irq_reload_ = true;
    irq_counter_ = 0;
}

void Mapper_004::irq_disable(uint8_t data)
{
    // Writing any value to this register will disable MMC3 interrupts
    // AND acknowledge any pending interrupts.
    irq_enabled_ = false;
}

void Mapper_004::irq_enable(uint8_t data)
{
    // Writing any value to this register will enable MMC3 interrupts.
    irq_enabled_ = true;
}

void Mapper_004::do_ppu_clock(int cycle, int scanline)
{
    if (cycle != 261)
        return;

    if ((scanline < 0 || scanline > 239) && scanline != 260)
        return;

    // 1. When the IRQ is clocked (filtered A12 0->1), the counter value is
    // checked - if zero or the reload flag is true, it's reloaded with
    // the IRQ latched value at $C000; otherwise, it decrements.
    if (irq_counter_ == 0 || irq_reload_) {
        irq_counter_ = irq_latch_;
        irq_reload_ = false;
    }
    else {
        irq_counter_--;
    }

    // 2. If the IRQ counter is zero and IRQs are enabled ($E001), an IRQ is
    // triggered. The "alternate revision" checks the IRQ counter
    // transition 1->0, whether from decrementing or reloading.
    if (irq_counter_ == 0 && irq_enabled_) {
        set_irq();
    }
}

} // namespace
