#ifndef MAPPER_001_H
#define MAPPER_001_H

#include "mapper.h"

namespace nes {

class Mapper_001 : public Mapper {
public:
    Mapper_001(const std::vector<uint8_t> &prg_rom,
            const std::vector<uint8_t> &chr_rom);
    virtual ~Mapper_001();

private:
    uint16_t prg_nbanks_ = 1;
    uint16_t chr_nbanks_ = 1;

    uint8_t shift_register_ = 0;
    uint8_t control_register_ = 0;

    uint8_t mirroring_ = 0;
    uint8_t prg_mode_ = 0;
    uint8_t chr_mode_ = 0;
    bool use_chr_ram_ = false;
    bool prg_ram_disabled_ = false;

    uint16_t prg_bank_0_ = 0;
    uint16_t prg_bank_1_ = 0;
    uint16_t chr_bank_0_ = 0;
    uint16_t chr_bank_1_ = 0;
    uint16_t prg_ram_bank_ = 0;

    void select_board();
    std::function<void(uint8_t)> write_chr_bank_0 = [](uint8_t){};
    std::function<void(uint8_t)> write_chr_bank_1 = [](uint8_t){};

    void write_control(uint8_t data);
    void write_prg_bank(uint8_t data);

    virtual uint8_t do_read_prg(uint16_t addr) const final;
    virtual void do_write_prg(uint16_t addr, uint8_t data) final;
    virtual uint8_t do_read_chr(uint16_t addr) const final;
    virtual void do_write_chr(uint16_t addr, uint8_t data) final;
};

} // namespace

#endif // _H
