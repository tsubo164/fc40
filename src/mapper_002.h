#ifndef MAPPER_002_H
#define MAPPER_002_H

#include "mapper.h"

namespace nes {

extern void open_mapper_002(Mapper *m, size_t prog_size, size_t char_size);

class Mapper_002 : public Mapper {
public:
    Mapper_002(const uint8_t *prog_rom, size_t prog_size,
           const uint8_t *char_rom, size_t char_size);
    virtual ~Mapper_002();

private:
    int prog_nbanks_ = 1;
    int char_nbanks_ = 1;

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
