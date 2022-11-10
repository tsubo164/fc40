#ifndef MAPPER_000_H
#define MAPPER_000_H

#include "mapper.h"

namespace nes {

extern void open_mapper_000(Mapper *m, size_t prog_size, size_t char_size);

class Mapper_000 : public Mapper {
public:
    Mapper_000(const uint8_t *prog_rom, size_t prog_size,
           const uint8_t *char_rom, size_t char_size);
    virtual ~Mapper_000();

private:
    int prog_nbanks_ = 1;

    virtual uint8_t do_read_prog(uint16_t addr) const final;
    virtual void do_write_prog(uint16_t addr, uint8_t data) final;
    virtual uint8_t do_read_char(uint16_t addr) const final;
    virtual void do_write_char(uint16_t addr, uint8_t data) final;
};

} // namespace

#endif // _H
