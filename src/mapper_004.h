#ifndef MAPPER_004_H
#define MAPPER_004_H

#include "mapper.h"
#include "bank_map.h"

namespace nes {

class Mapper_004 : public Mapper {
public:
    Mapper_004(const std::vector<uint8_t> &prg_rom,
            const std::vector<uint8_t> &chr_rom);
    virtual ~Mapper_004();

private:
    bankmap<Size::_8KB,4> prg_;
    bankmap<Size::_1KB,8> chr_;

    int bank_select_ = 0;
    int prg_bank_mode_ = 0;
    int chr_bank_mode_ = 0;

    int irq_counter_ = 0;
    int irq_latch_ = 0;
    bool irq_enabled_ = false;
    bool irq_reload_ = true;

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

    void do_ppu_clock(int cycle, int scanline) override final;
};

} // namespace

#endif // _H
