#ifndef MAPPER_016_H
#define MAPPER_016_H

#include "mapper.h"
#include "bank_map.h"

namespace nes {

class Mapper_016 : public Mapper {
public:
    Mapper_016(const std::vector<uint8_t> &prg_rom,
            const std::vector<uint8_t> &chr_rom);
    virtual ~Mapper_016();

private:
    bank_map<Size::_16KB,2> prg_;
    bank_map<Size::_1KB,8>  chr_;

    int submapper_ = 5;

    bool irq_enabled_ = false;
    uint16_t irq_counter_ = 0;
    uint16_t irq_latch_ = 0;

    uint8_t do_read_prg(uint16_t addr) const override final;
    uint8_t do_read_chr(uint16_t addr) const override final;
    void do_write_prg(uint16_t addr, uint8_t data) override final;
    void do_write_chr(uint16_t addr, uint8_t data) override final;

    void do_cpu_clock() override final;
};

} // namespace

#endif // _H
