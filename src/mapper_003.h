#ifndef MAPPER_003_H
#define MAPPER_003_H

#include "mapper.h"
#include "bank_map.h"

namespace nes {

class Mapper_003 : public Mapper {
public:
    Mapper_003(const std::vector<uint8_t> &prg_rom,
            const std::vector<uint8_t> &chr_rom);
    virtual ~Mapper_003();

private:
    bankmap<Size::_16KB,2> prg_;
    bankmap<Size::_8KB,1>  chr_;

    uint8_t do_read_prg(uint16_t addr) const override final;
    uint8_t do_read_chr(uint16_t addr) const override final;
    void do_write_prg(uint16_t addr, uint8_t data) override final;
    void do_write_chr(uint16_t addr, uint8_t data) override final;
};

} // namespace

#endif // _H
