#ifndef MAPPER_019_H
#define MAPPER_019_H

#include "mapper.h"
#include "bank_map.h"
#include <array>

namespace nes {

class Mapper_019 : public Mapper {
public:
    Mapper_019(const std::vector<uint8_t> &prg_rom,
            const std::vector<uint8_t> &chr_rom);
    ~Mapper_019();

private:
    BankMap prg_banks_;
    BankMap chr_banks_;

    bool use_ntram_lo_ = false;
    bool use_ntram_hi_ = false;
    uint16_t irq_counter_ = 0;
    bool irq_enabled_ = false;

    enum class NTRAM {
        No = 0,
        Lo = 1,
        Hi = 2,
    };
    std::array<NTRAM,12> ntram_select_ = { NTRAM::No };

    // These ASICs have the unusual ability to select the internal
    // 2 KB nametable RAM as a CHR bank page, allowing it to be used as
    // CHR RAM in combination with the existing CHR ROM.
    //std::array<uint8_t,0x800> ntram_;

    uint8_t do_read_prg(uint16_t addr) const override final;
    uint8_t do_read_chr(uint16_t addr) const override final;
    void do_write_prg(uint16_t addr, uint8_t data) override final;
    void do_write_chr(uint16_t addr, uint8_t data) override final;

    void do_cpu_clock() override final;
};

} // namespace

#endif // _H
