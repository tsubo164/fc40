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
    bankmap<Size::_8KB,4>  prg_;
    bankmap<Size::_1KB,12> chr_; // CHR window 1K x 8(PT) + 1K x 4(NT)

    bool enable_nt_lo_ = false;
    bool enable_nt_hi_ = false;
    int irq_counter_ = 0;
    bool irq_enabled_ = false;

    enum Select {
        SELECT_CHR_ROM = 0,
        SELECT_NTRAM_LO = 1,
        SELECT_NTRAM_HI = 2,
    };
    std::array<int,12> bank_select_ = { SELECT_CHR_ROM };

    uint8_t read_chr(uint16_t addr) const;
    void write_chr(uint16_t addr, uint8_t data);

    uint8_t do_read_prg(uint16_t addr) const override final;
    uint8_t do_read_chr(uint16_t addr) const override final;
    uint8_t do_read_nametable(uint16_t addr) const override final;
    void do_write_prg(uint16_t addr, uint8_t data) override final;
    void do_write_chr(uint16_t addr, uint8_t data) override final;
    void do_write_nametable(uint16_t addr, uint8_t data) override final;

    void do_cpu_clock() override final;
};

} // namespace

#endif // _H
