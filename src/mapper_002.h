#ifndef MAPPER_002_H
#define MAPPER_002_H

#include "mapper.h"

namespace nes {

class Mapper_002 : public Mapper {
public:
    Mapper_002(const std::vector<uint8_t> &prg_rom,
            const std::vector<uint8_t> &chr_rom);
    virtual ~Mapper_002();

private:
    uint16_t prg_bank_ = 0;
    uint16_t prg_fixed_ = 0;

    virtual uint8_t do_read_prg(uint16_t addr) const final;
    virtual void do_write_prg(uint16_t addr, uint8_t data) final;
    virtual uint8_t do_read_chr(uint16_t addr) const final;
    virtual void do_write_chr(uint16_t addr, uint8_t data) final;
};

} // namespace

#endif // _H
