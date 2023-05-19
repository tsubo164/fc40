#ifndef MAPPER_000_H
#define MAPPER_000_H

#include "mapper.h"

namespace nes {

class Mapper_000 : public Mapper {
public:
    Mapper_000(const std::vector<uint8_t> &prog_rom,
            const std::vector<uint8_t> &char_rom);
    virtual ~Mapper_000();

private:
    uint16_t mirroring_mask_ = 0x7FFF;

    virtual uint8_t do_read_prog(uint16_t addr) const final;
    virtual void do_write_prog(uint16_t addr, uint8_t data) final;
    virtual uint8_t do_read_char(uint16_t addr) const final;
    virtual void do_write_char(uint16_t addr, uint8_t data) final;
};

} // namespace

#endif // _H
