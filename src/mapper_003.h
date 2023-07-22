#ifndef MAPPER_003_H
#define MAPPER_003_H

#include "mapper.h"

namespace nes {

class Mapper_003 : public Mapper {
public:
    Mapper_003(const std::vector<uint8_t> &prg_rom,
            const std::vector<uint8_t> &chr_rom);
    virtual ~Mapper_003();

private:
    uint16_t prg_mirroring_mask_ = 0x7FFF;
    int chr_bank_ = 0;

    virtual uint8_t do_read_prg(uint16_t addr) const final;
    virtual void do_write_prg(uint16_t addr, uint8_t data) final;
    virtual uint8_t do_read_chr(uint16_t addr) const final;
    virtual void do_write_chr(uint16_t addr, uint8_t data) final;
};

} // namespace

#endif // _H
