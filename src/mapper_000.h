#ifndef MAPPER_000_H
#define MAPPER_000_H

#include "mapper.h"

namespace nes {

class Mapper_000 : public Mapper {
public:
    Mapper_000(const std::vector<uint8_t> &prg_rom,
            const std::vector<uint8_t> &chr_rom);
    virtual ~Mapper_000();

private:
    bank_map<Size::_16KB,2> prg_;

    uint8_t do_read_prg(uint16_t addr) const override final;
    uint8_t do_read_chr(uint16_t addr) const override final;
    void do_write_prg(uint16_t addr, uint8_t data) override final;
    void do_write_chr(uint16_t addr, uint8_t data) override final;

    void do_get_prg_bank_info(BankInfo &info) const override;
    void do_get_chr_bank_info(BankInfo &ifno) const override;
};

} // namespace

#endif // _H
