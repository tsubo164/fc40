#ifndef MAPPER_004_H
#define MAPPER_004_H

#include "mapper.h"

namespace nes {

class Mapper_004 : public Mapper {
public:
    Mapper_004(const std::vector<uint8_t> &prg_rom,
            const std::vector<uint8_t> &chr_rom);
    virtual ~Mapper_004();

private:
    uint8_t prg_bank_[4] = {};
    uint8_t chr_bank_[8] = {};

    bool use_chr_ram_ = false;
    uint8_t bank_select_ = 0;
    uint8_t prg_bank_mode_ = 0;
    uint8_t chr_bank_mode_ = 0;
    uint8_t prg_bank_count_ = 0;
    uint8_t chr_bank_count_ = 0;

    uint8_t mirroring_ = 0;
    uint8_t irq_latch_ = 0;

    void set_bank_select(uint16_t addr, uint8_t data);
    void set_bank_data(uint16_t addr, uint8_t data);
    void set_mirroring(uint8_t data);
    void set_prg_ram_protect(uint8_t data);
    void set_irq_latch(uint8_t data);
    void set_irq_reload(uint8_t data);
    void irq_disable(uint8_t data);
    void irq_enable(uint8_t data);

    virtual uint8_t do_read_prg(uint16_t addr) const final;
    virtual uint8_t do_read_chr(uint16_t addr) const final;
    virtual void do_write_prg(uint16_t addr, uint8_t data) final;
    virtual void do_write_chr(uint16_t addr, uint8_t data) final;
};

} // namespace

#endif // _H