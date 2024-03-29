#ifndef MAPPER_016_H
#define MAPPER_016_H

#include "mapper.h"

namespace nes {

class Mapper_016 : public Mapper {
public:
    Mapper_016(const std::vector<uint8_t> &prg_rom,
            const std::vector<uint8_t> &chr_rom);
    virtual ~Mapper_016();

private:
    bank_map<Size::_16KB,2> prg_;
    bank_map<Size::_1KB,8>  chr_;

    bool irq_enabled_ = false;
    uint16_t irq_counter_ = 0;
    uint16_t irq_latch_ = 0;

    int submapper_ = 5;

    // serialization
    void do_serialize(Archive &ar) override final
    {
        SERIALIZE(ar, this, prg_);
        SERIALIZE(ar, this, chr_);
        SERIALIZE(ar, this, irq_enabled_);
        SERIALIZE(ar, this, irq_counter_);
        SERIALIZE(ar, this, irq_latch_);
        SERIALIZE(ar, this, submapper_);
    }

    uint8_t do_read_prg(uint16_t addr) const override final;
    uint8_t do_read_chr(uint16_t addr) const override final;
    void do_write_prg(uint16_t addr, uint8_t data) override final;
    void do_write_chr(uint16_t addr, uint8_t data) override final;

    void do_get_prg_bank_info(BankInfo &info) const override;
    void do_get_chr_bank_info(BankInfo &ifno) const override;

    void do_cpu_clock() override final;
};

} // namespace

#endif // _H
