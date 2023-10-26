#include "mapper_016.h"

namespace nes {

Mapper_016::Mapper_016(const std::vector<uint8_t> &prg_rom,
        const std::vector<uint8_t> &chr_rom) : Mapper(prg_rom, chr_rom)
{
    prg_.resize(GetPrgRomSize());
    chr_.resize(GetChrRomSize());

    prg_.select(1, -1);
    use_prg_ram(0x2000);
}

Mapper_016::~Mapper_016()
{
}

uint8_t Mapper_016::do_read_prg(uint16_t addr) const
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

uint8_t Mapper_016::do_read_chr(uint16_t addr) const
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

void Mapper_016::do_write_prg(uint16_t addr, uint8_t data)
{
    int index = 0;

    switch (submapper_) {
    case 4:
        index = addr - 0x6000;
        break;
    case 5:
        index = addr - 0x8000; 
        break;
    default:
        break;
    }

    if (index >= 0x0000 && index <= 0x0007) {
        // CHR-ROM Bank Select ($6000-$6007 write, Submapper 4; $8000-$8007 write, Submapper 5)
        // Mask: $E00F (Submapper 4), $800F (Submapper 5)
        // 7  bit  0
        // ---- ----
        // CCCC CCCC
        // |||| ||||
        // ++++-++++-- 1 KiB CHR-ROM bank number
        // $xxx0: Select 1 KiB CHR-ROM bank at PPU $0000-$03FF
        // $xxx1: Select 1 KiB CHR-ROM bank at PPU $0400-$07FF
        // $xxx2: Select 1 KiB CHR-ROM bank at PPU $0800-$0BFF
        // $xxx3: Select 1 KiB CHR-ROM bank at PPU $0C00-$0FFF
        // $xxx4: Select 1 KiB CHR-ROM bank at PPU $1000-$13FF
        // $xxx5: Select 1 KiB CHR-ROM bank at PPU $1400-$17FF
        // $xxx6: Select 1 KiB CHR-ROM bank at PPU $1800-$1BFF
        // $xxx7: Select 1 KiB CHR-ROM bank at PPU $1C00-$1FFF
        const int window = index & 0x7;
        chr_.select(window, data);
    }
    else if (index == 0x0008) {
        // PRG-ROM Bank Select ($6008 write, Submapper 4; $8008 write, Submapper 5)
        // Mask: $E00F (Submapper 4), $800F (Submapper 5)
        // 7  bit  0
        // ---- ----
        // .... PPPP
        //      ||||
        //      ++++-- Select 16 KiB PRG-ROM bank at CPU $8000-$BFFF   
        prg_.select(0, data & 0xF);
    }
    else if (index == 0x0009) {
        // Nametable Mirroring Type Select ($6009 write, Submapper 4; $8009 write, Submapper 5)
        // Mask: $E00F (Submapper 4), $800F (Submapper 5)
        // 7  bit  0
        // ---- ----
        // .... ..MM
        //        ||
        //        ++-- Select nametable mirroring type
        //              0: Vertical
        //              1: Horizontal
        //              2: One-screen, page 0
        //              3: One-screen, page 1
        switch (data & 0x3) {
        case 0:
            SetMirroring(MIRRORING_VERTICAL);
            break;
        case 1:
            SetMirroring(MIRRORING_HORIZONTAL);
            break;
        case 2:
            SetMirroring(MIRRORING_SINGLE_SCREEN_0);
            break;
        case 3:
            SetMirroring(MIRRORING_SINGLE_SCREEN_1);
            break;
        default:
            break;
        }
    }
    else if (index == 0x000A) {
        // IRQ Control ($600A write, Submapper 4; $800A write, Submapper 5)
        // Mask: $E00F (Submapper 4), $800F (Submapper 5)
        // 7  bit  0
        // ---- ----
        // .... ...C
        //         |
        //         +-- IRQ counter control
        //              0: Counting disabled
        //              1: Counting enabled
        // - Writing to this register acknowledges a pending IRQ.
        // - On the LZ93D50 (Submapper 5), writing to this register also copies
        //   the latch to the actual counter.
        // - If a write to this register enables counting while the counter is
        //   holding a value of zero, an IRQ is generated immediately.
        irq_enabled_ = data & 0x1;
        ClearIRQ();

        if (irq_enabled_)
            if (irq_counter_ == 0)
                set_irq();

        if (submapper_ == 5)
            irq_counter_ = irq_latch_;
    }
    else if (index == 0x000B) {
        // IRQ Latch/Counter ($600B-$600C write, Submapper 4; $800B-$800C write, Submapper 5)
        // Mask: $E00F (Submapper 4), $800F (Submapper 5)
        // 
        //    $C         $B
        // 7  bit  0  7  bit  0
        // ---- ----  ---- ----
        // CCCC CCCC  CCCC CCCC
        // |||| ||||  |||| ||||
        // ++++-++++--++++-++++-- Counter value (little-endian)
        // - If counting is enabled, the counter decreases on every M2 cycle.
        //   When it holds a value of zero, an IRQ is generated.
        // - On the FCG-1/2 (Submapper 4), writing to these two registers directly
        //   modifies the counter itself; all such games therefore disable counting
        //   before changing the counter value.
        // - On the LZ93D50 (Submapper 5), these registers instead modify a latch
        //   that will only be copied to the actual counter when register $800A
        //   is written to.
        if (submapper_ == 4)
            irq_counter_ = (irq_counter_ & 0xFF00) | data;
        else if (submapper_ == 5)
            irq_latch_ = (irq_latch_ & 0xFF00) | data;
    }
    else if (index == 0x000C) {
        if (submapper_ == 4)
            irq_counter_ = (irq_counter_ & 0x00FF) | (data << 8);
        else if (submapper_ == 5)
            irq_latch_ = (irq_latch_ & 0x00FF) | (data << 8);
    }
}

void Mapper_016::do_write_chr(uint16_t addr, uint8_t data)
{
}

void Mapper_016::do_cpu_clock()
{
    if (!irq_enabled_)
        return;

    if (irq_counter_ == 0)
        set_irq();
    else
        irq_counter_--;
}

} // namespace
