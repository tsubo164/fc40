#ifndef MAPPER_002_H
#define MAPPER_002_H

#include "mapper.h"

namespace nes {

class Mapper_002 : public Mapper {
public:
    Mapper_002(const std::vector<uint8_t> &prog_rom,
            const std::vector<uint8_t> &char_rom);
    virtual ~Mapper_002();

private:
    int prog_bank_ = 0;
    int prog_fixed_ = 0;
    uint8_t char_ram_[0x2000] = {0x00};

    virtual uint8_t do_read_prog(uint16_t addr) const final;
    virtual void do_write_prog(uint16_t addr, uint8_t data) final;
    virtual uint8_t do_read_char(uint16_t addr) const final;
    virtual void do_write_char(uint16_t addr, uint8_t data) final;
};

} // namespace

#endif // _H
