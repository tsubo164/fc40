#include "mapper_004.h"

namespace nes {

Mapper_004::Mapper_004(const std::vector<uint8_t> &prg_rom,
        const std::vector<uint8_t> &chr_rom) : Mapper(prg_rom, chr_rom)
{
    const int prg_bank_count = GetPrgRomSize() / 0x2000; // 8KB

    prg_bank_[0] = 0;
    prg_bank_[1] = 1;
    prg_bank_[2] = prg_bank_count - 2;
    prg_bank_[3] = prg_bank_count - 1;
    printf("prg_bank_count %d\n", prg_bank_count);
    printf("prg_bank_count %d\n", prg_bank_[3]);
    printf("prg_bank_count %04X\n", prg_bank_[3] * 0x2000);

    chr_bank_[0] = 0;
    chr_bank_[1] = 1;
    chr_bank_[2] = 2;
    chr_bank_[3] = 3;
    chr_bank_[4] = 4;
    chr_bank_[5] = 5;
    chr_bank_[6] = 6;
    chr_bank_[7] = 7;

    // TODO confirm
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
        const uint32_t a = addr - 0x6000;
        return read_prg_ram(a);
    }
    else if (addr >= 0x8000 && addr <= 0x9FFF) {
        const uint32_t a = prg_bank_[0] * 0x2000 + (addr & 0x1FFF);
        return read_prg_rom(a);
    }
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        const uint32_t a = prg_bank_[1] * 0x2000 + (addr & 0x1FFF);
        return read_prg_rom(a);
    }
    else if (addr >= 0xC000 && addr <= 0xDFFF) {
        const uint32_t a = prg_bank_[2] * 0x2000 + (addr & 0x1FFF);
        return read_prg_rom(a);
    }
    else if (addr >= 0xE000 && addr <= 0xFFFF) {
        const uint32_t a = prg_bank_[3] * 0x2000 + (addr & 0x1FFF);
        return read_prg_rom(a);
    }
    else {
        return 0xFF;
    }
}

uint8_t Mapper_004::do_read_chr(uint16_t addr) const
{
    if (addr >= 0x0000 && addr <= 0x03FF) {
        const uint32_t a = chr_bank_[0] * 0x0400 + (addr & 0x03FF);
        return read_chr_rom(a);
    }
    else if (addr >= 0x0400 && addr <= 0x07FF) {
        const uint32_t a = chr_bank_[1] * 0x0400 + (addr & 0x03FF);
        return read_chr_rom(a);
    }
    else if (addr >= 0x0800 && addr <= 0x0BFF) {
        const uint32_t a = chr_bank_[2] * 0x0400 + (addr & 0x03FF);
        return read_chr_rom(a);
    }
    else if (addr >= 0x0C00 && addr <= 0x0FFF) {
        const uint32_t a = chr_bank_[3] * 0x0400 + (addr & 0x03FF);
        return read_chr_rom(a);
    }
    else if (addr >= 0x1000 && addr <= 0x13FF) {
        const uint32_t a = chr_bank_[4] * 0x0400 + (addr & 0x03FF);
        return read_chr_rom(a);
    }
    else if (addr >= 0x1400 && addr <= 0x17FF) {
        const uint32_t a = chr_bank_[5] * 0x0400 + (addr & 0x03FF);
        return read_chr_rom(a);
    }
    else if (addr >= 0x1800 && addr <= 0x1BFF) {
        const uint32_t a = chr_bank_[6] * 0x0400 + (addr & 0x03FF);
        return read_chr_rom(a);
    }
    else if (addr >= 0x1C00 && addr <= 0x1FFF) {
        const uint32_t a = chr_bank_[7] * 0x0400 + (addr & 0x03FF);
        return read_chr_rom(a);
    }
    else {
        return 0xFF;
    }
}

void Mapper_004::do_write_prg(uint16_t addr, uint8_t data)
{
    const bool even = addr % 2 == 0;

    if (addr >= 0x8000 && addr <= 0x9FFF) {
        if (even)
            set_bank_select(addr, data);
        else
            set_bank_data(addr, data);
    }
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (even)
            set_mirroring(data);
    }
    else if (addr >= 0xC000 && addr <= 0xDFFF) {
        if (even)
            set_irq_latch(data);
        else
            set_irq_reload(data);
    }
    else if (addr >= 0xE000 && addr <= 0xFFFF) {
    }
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
            chr_bank_[0] = (data & 0xFE);
            chr_bank_[1] = (data & 0xFE) + 1;
            break;
        case 1:
            chr_bank_[2] = (data & 0xFE);
            chr_bank_[3] = (data & 0xFE) + 1;
            break;
        case 2:
            chr_bank_[4] = data;
            break;
        case 3:
            chr_bank_[5] = data;
            break;
        case 4:
            chr_bank_[6] = data;
            break;
        case 5:
            chr_bank_[7] = data;
            break;
        }
    }
    else {
        switch (bank_select_) {
        case 0:
            chr_bank_[0] = data;
            break;
        case 1:
            chr_bank_[1] = data;
            break;
        case 2:
            chr_bank_[2] = data;
            break;
        case 3:
            chr_bank_[3] = data;
            break;
        case 4:
            chr_bank_[4] = (data & 0xFE);
            chr_bank_[5] = (data & 0xFE) + 1;
            break;
        case 5:
            chr_bank_[6] = (data & 0xFE);
            chr_bank_[7] = (data & 0xFE) + 1;
            break;
        }
    }
    if (prg_bank_mode_ == 0) {
        const int prg_bank_count = GetPrgRomSize() / 0x2000; // 8KB

        switch (bank_select_) {
        case 6:
            prg_bank_[0] = data;
            prg_bank_[2] = prg_bank_count - 2;
            prg_bank_[3] = prg_bank_count - 1;
            break;
        case 7:
            prg_bank_[1] = data;
            prg_bank_[2] = prg_bank_count - 2;
            prg_bank_[3] = prg_bank_count - 1;
            break;
        }
    }
    else {
        const int prg_bank_count = GetPrgRomSize() / 0x2000; // 8KB

        switch (bank_select_) {
        case 6:
            prg_bank_[2] = data;
            prg_bank_[0] = prg_bank_count - 2;
            prg_bank_[3] = prg_bank_count - 1;
            break;
        case 7:
            prg_bank_[1] = data;
            prg_bank_[0] = prg_bank_count - 2;
            prg_bank_[3] = prg_bank_count - 1;
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
    mirroring_ = data & 0x01;
}

void Mapper_004::set_prg_ram_protect(uint8_t data)
{
}

void Mapper_004::set_irq_latch(uint8_t data)
{
    irq_latch_ = data;
}

void Mapper_004::set_irq_reload(uint8_t data)
{
}

void Mapper_004::irq_disable(uint8_t data)
{
}

void Mapper_004::irq_enable(uint8_t data)
{
}

void Mapper_004::do_write_chr(uint16_t addr, uint8_t data)
{
}

} // namespace
