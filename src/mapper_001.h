#ifndef MAPPER_001_H
#define MAPPER_001_H

#include "mapper.h"
#include "bank_map.h"

namespace nes {

class Mapper_001 : public Mapper {
public:
    Mapper_001(const std::vector<uint8_t> &prg_rom,
            const std::vector<uint8_t> &chr_rom);
    virtual ~Mapper_001();

private:
    bank_map<Size::_16KB,2> prg_;
    bank_map<Size::_4KB,2>  chr_;
    uint16_t prg_ram_bank_ = 0;

    uint8_t shift_register_ = 0;
    uint8_t control_register_ = 0;

    uint8_t prg_mode_ = 0;
    uint8_t chr_mode_ = 0;
    bool prg_ram_disabled_ = false;
    int mmc3_board_ = 0;

    void select_board();
    void write_chr_bank(int window, uint8_t data);

    void write_control(uint8_t data);
    void write_prg_bank(uint8_t data);

    uint8_t do_read_prg(uint16_t addr) const override final;
    uint8_t do_read_chr(uint16_t addr) const override final;
    void do_write_prg(uint16_t addr, uint8_t data) override final;
    void do_write_chr(uint16_t addr, uint8_t data) override final;
};

} // namespace

#endif // _H
