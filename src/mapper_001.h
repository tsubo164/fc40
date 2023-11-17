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
    bank_map<Size::_16KB,2> prg_;
    bank_map<Size::_4KB,2>  chr_;
    uint16_t prg_ram_bank_ = 0;

    uint8_t shift_register_ = 0;
    uint8_t control_register_ = 0;

    uint8_t prg_mode_ = 0;
    uint8_t chr_mode_ = 0;
    bool prg_ram_disabled_ = false;
    int mmc3_board_ = 0;

    // serialization
    void do_serialize(Archive &ar) override final
    {
        SERIALIZE(ar, this, prg_);
        SERIALIZE(ar, this, chr_);
        SERIALIZE(ar, this, prg_ram_bank_);
        SERIALIZE(ar, this, shift_register_);
        SERIALIZE(ar, this, control_register_);
        SERIALIZE(ar, this, prg_mode_);
        SERIALIZE(ar, this, chr_mode_);
        SERIALIZE(ar, this, prg_ram_disabled_);
        SERIALIZE(ar, this, mmc3_board_);
    }

    void select_board();
    void write_chr_bank(int window, uint8_t data);

    void write_control(uint8_t data);
    void write_prg_bank(uint8_t data);

    uint8_t do_read_prg(uint16_t addr) const override final;
    uint8_t do_read_chr(uint16_t addr) const override final;
    void do_write_prg(uint16_t addr, uint8_t data) override final;
    void do_write_chr(uint16_t addr, uint8_t data) override final;

    void do_get_prg_bank_info(BankInfo &info) const override;
    void do_get_chr_bank_info(BankInfo &ifno) const override;
};

} // namespace

#endif // _H
