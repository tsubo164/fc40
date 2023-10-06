#ifndef MAPPER_002_H
#define MAPPER_002_H

#include "mapper.h"
#include "bank_map.h"

namespace nes {

class Mapper_002 : public Mapper {
public:
    Mapper_002(const std::vector<uint8_t> &prg_rom,
            const std::vector<uint8_t> &chr_rom);
    virtual ~Mapper_002();

private:
    bank_map<Size::_16KB,2> prg_;

    uint8_t do_read_prg(uint16_t addr) const override final;
    uint8_t do_read_chr(uint16_t addr) const override final;
    void do_write_prg(uint16_t addr, uint8_t data) override final;
    void do_write_chr(uint16_t addr, uint8_t data) override final;
};

} // namespace

#endif // _H
