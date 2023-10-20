#include "mapper_076.h"

namespace nes {

Mapper_076::Mapper_076(const std::vector<uint8_t> &prg_rom,
        const std::vector<uint8_t> &chr_rom) : Mapper(prg_rom, chr_rom)
{
    prg_.resize(GetPrgRomSize());
    chr_.resize(GetChrRomSize());
    prg_.select(2, -2);
    prg_.select(3, -1);
}

Mapper_076::~Mapper_076()
{
}

uint8_t Mapper_076::do_read_prg(uint16_t addr) const
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

uint8_t Mapper_076::do_read_chr(uint16_t addr) const
{
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        const int index = chr_.map(addr);
        return read_chr_rom(index);
    }
    else {
        return 0xFF;
    }
}

void Mapper_076::do_write_prg(uint16_t addr, uint8_t data)
{
    if (addr == 0x8000) {
        // Registers:
        //  ---------------------------
        //  Mask: $E001
        //  
        //    $8000:  [.... .AAA]
        //      A = Address for use with $8001
        //  
        //    $8001:  [..DD DDDD]    Data port:
        //        R:2 ->  CHR reg 0  (2k @ $0000)
        //        R:3 ->  CHR reg 1  (2k @ $0800)
        //        R:4 ->  CHR reg 2  (2k @ $1000)
        //        R:5 ->  CHR reg 3  (2k @ $1800)
        //        R:6 ->  PRG reg 0  (8k @ $8000)
        //        R:7 ->  PRG reg 1  (8k @ $a000)
        //  
        //  CHR Setup:
        //  ---------------------------
        //        $0000   $0400   $0800   $0C00   $1000   $1400   $1800   $1C00 
        //      +---------------+---------------+---------------+---------------+
        //      |      R:2      |      R:3      |      R:4      |      R:5      |
        //      +---------------+---------------+---------------+---------------+
        //  
        //  PRG Setup:
        //  ---------------------------
        //        $8000   $A000   $C000   $E000  
        //      +-------+-------+-------+-------+
        //      |  R:6  |  R:7  | { -2} | { -1} |
        //      +-------+-------+-------+-------+
        bank_select_ = data & 0x07;
    }
    else if (addr == 0x8001) {
        const int bank_data = data & 0x3F;

        switch (bank_select_) {
        case 2:
            chr_.select(0, bank_data);
            break;
        case 3:
            chr_.select(1, bank_data);
            break;
        case 4:
            chr_.select(2, bank_data);
            break;
        case 5:
            chr_.select(3, bank_data);
            break;
        case 6:
            prg_.select(0, bank_data);
            break;
        case 7:
            prg_.select(1, bank_data);
            break;
        default:
            break;
        }
    }
}

void Mapper_076::do_write_chr(uint16_t addr, uint8_t data)
{
}

} // namespace
