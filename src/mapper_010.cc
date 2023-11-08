#include "mapper_010.h"

namespace nes {

Mapper_010::Mapper_010(const std::vector<uint8_t> &prg_rom,
        const std::vector<uint8_t> &chr_rom) : Mapper(prg_rom, chr_rom)
{
    prg_.resize(GetPrgRomSize());
    chr_fd_.resize(GetChrRomSize());
    chr_fe_.resize(GetChrRomSize());
    prg_.select(1, -1);

    use_prg_ram(0x2000);

    latch_0_ = 0xFE;
    latch_1_ = 0xFE;
}

Mapper_010::~Mapper_010()
{
}

uint8_t Mapper_010::do_read_prg(uint16_t addr) const
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
        return 0xFF;
    }
}

uint8_t Mapper_010::do_read_chr(uint16_t addr) const
{
	uint8_t data = 0xFF;

    if (addr >= 0x0000 && addr <= 0x0FFF) {
        if (latch_0_ == 0xFD) {
            const int index = chr_fd_.map(addr);
            data = read_chr_rom(index);
        }
        if (latch_0_ == 0xFE) {
            const int index = chr_fe_.map(addr);
            data = read_chr_rom(index);
        }

		// PPU reads $0FD8 through $0FDF: latch 0 is set to $FD
		if (addr >= 0x0FD8 && addr <= 0x0FDF) {
			const_cast<Mapper_010*>(this)->latch_0_ = 0xFD;
		}
		// PPU reads $0FE8 through $0FEF: latch 0 is set to $FE
		if (addr >= 0x0FE8 && addr <= 0x0FEF) {
			const_cast<Mapper_010*>(this)->latch_0_ = 0xFE;
		}
    }
    else if (addr >= 0x1000 && addr <= 0x1FFF) {
        if (latch_1_ == 0xFD) {
            const int index = chr_fd_.map(addr);
            data = read_chr_rom(index);
        }
        if (latch_1_ == 0xFE) {
            const int index = chr_fe_.map(addr);
            data = read_chr_rom(index);
        }

		// PPU reads $1FD8 through $1FDF: latch 1 is set to $FD
		if (addr >= 0x1FD8 && addr <= 0x1FDF) {
			const_cast<Mapper_010*>(this)->latch_1_ = 0xFD;
		}
		// PPU reads $1FE8 through $1FEF: latch 1 is set to $FE
		if (addr >= 0x1FE8 && addr <= 0x1FEF) {
			const_cast<Mapper_010*>(this)->latch_1_ = 0xFE;
		}
    }

    return data;
}

void Mapper_010::do_write_prg(uint16_t addr, uint8_t data)
{
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        const int index = addr - 0x6000;
        write_prg_ram(index, data);
    }
    else if (addr >= 0xA000 && addr <= 0xAFFF) {
        // PRG ROM bank select ($A000-$AFFF)
        // 7  bit  0
        // ---- ----
        // xxxx PPPP
        //      ||||
        //      ++++- Select 8 KB PRG ROM bank for CPU $8000-$9FFF
        prg_.select(0, data & 0x0F);
    }
    else if (addr >= 0xB000 && addr <= 0xBFFF) {
        // CHR ROM $FD/0000 bank select ($B000-$BFFF)
        // 7  bit  0
        // ---- ----
        // xxxC CCCC
        //    | ||||
        //    +-++++- Select 4 KB CHR ROM bank for PPU $0000-$0FFF
        //            used when latch 0 = $FD
        chr_fd_.select(0, data & 0x1F);
    }
    else if (addr >= 0xC000 && addr <= 0xCFFF) {
        // CHR ROM $FE/0000 bank select ($C000-$CFFF)
        // 7  bit  0
        // ---- ----
        // xxxC CCCC
        //    | ||||
        //    +-++++- Select 4 KB CHR ROM bank for PPU $0000-$0FFF
        //            used when latch 0 = $FE
        chr_fe_.select(0, data & 0x1F);
    }
    else if (addr >= 0xD000 && addr <= 0xDFFF) {
        // CHR ROM $FD/1000 bank select ($D000-$DFFF)
        // 7  bit  0
        // ---- ----
        // xxxC CCCC
        //    | ||||
        //    +-++++- Select 4 KB CHR ROM bank for PPU $1000-$1FFF
        //            used when latch 1 = $FD
        chr_fd_.select(1, data & 0x1F);
    }
    else if (addr >= 0xE000 && addr <= 0xEFFF) {
        // CHR ROM $FE/1000 bank select ($E000-$EFFF)
        // 7  bit  0
        // ---- ----
        // xxxC CCCC
        //    | ||||
        //    +-++++- Select 4 KB CHR ROM bank for PPU $1000-$1FFF
        //            used when latch 1 = $FE
        chr_fe_.select(1, data & 0x1F);
    }
    else if (addr >= 0xF000 && addr <= 0xFFFF) {
        // Mirroring ($F000-$FFFF)
        // 7  bit  0
        // ---- ----
        // xxxx xxxM
        //         |
        //         +- Select nametable mirroring (0: vertical; 1: horizontal)
        if ((data & 0x1) == 0)
            SetMirroring(MIRRORING_VERTICAL);
        else
            SetMirroring(MIRRORING_HORIZONTAL);
    }
}

void Mapper_010::do_write_chr(uint16_t addr, uint8_t data)
{
}

} // namespace
