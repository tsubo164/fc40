#ifndef MAPPER_004_H
#define MAPPER_004_H

#include "mapper.h"
#include <array>

namespace nes {

class Mapper_004 : public Mapper {
public:
    Mapper_004(const std::vector<uint8_t> &prg_rom,
            const std::vector<uint8_t> &chr_rom);
    virtual ~Mapper_004();

private:
    bank_map<Size::_8KB,4> prg_;
    bank_map<Size::_1KB,8> chr_;

    // registers
    std::array<uint8_t,8> bank_registers_ = {0};
    uint8_t bank_select_ = 0;
    uint8_t prg_bank_mode_ = 0;
    uint8_t chr_bank_mode_ = 0;

    uint8_t irq_counter_ = 0;
    uint8_t irq_latch_ = 0;
    bool irq_enabled_ = false;
    bool irq_reload_ = true;

    // serialization
    void do_serialize(Archive &ar) override final
    {
        SERIALIZE(ar, this, prg_);
        SERIALIZE(ar, this, chr_);
        SERIALIZE(ar, this, bank_registers_);
        SERIALIZE(ar, this, bank_select_);
        SERIALIZE(ar, this, prg_bank_mode_);
        SERIALIZE(ar, this, chr_bank_mode_);
        SERIALIZE(ar, this, irq_counter_);
        SERIALIZE(ar, this, irq_latch_);
        SERIALIZE(ar, this, irq_enabled_);
        SERIALIZE(ar, this, irq_reload_);
    }

    void update_prg_bank_mapping();
    void update_chr_bank_mapping();
    void set_bank_select(uint16_t addr, uint8_t data);
    void set_bank_data(uint16_t addr, uint8_t data);
    void set_mirroring(uint8_t data);
    void set_protection(uint8_t data);
    void set_irq_latch(uint8_t data);
    void set_irq_reload(uint8_t data);
    void irq_disable(uint8_t data);
    void irq_enable(uint8_t data);

    uint8_t do_read_prg(uint16_t addr) const override final;
    uint8_t do_read_chr(uint16_t addr) const override final;
    void do_write_prg(uint16_t addr, uint8_t data) override final;
    void do_write_chr(uint16_t addr, uint8_t data) override final;

    void do_get_prg_bank_info(BankInfo &info) const override;
    void do_get_chr_bank_info(BankInfo &ifno) const override;

    void do_ppu_clock(int cycle, int scanline) override final;
};

} // namespace

#endif // _H
