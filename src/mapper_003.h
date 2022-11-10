#ifndef MAPPER_003_H
#define MAPPER_003_H

#include "mapper.h"

namespace nes {

extern void open_mapper_003(Mapper *m, size_t prog_size, size_t char_size);

class Mapper_003 : public Mapper {
public:
    Mapper_003(const uint8_t *prog_rom, size_t prog_size,
           const uint8_t *char_rom, size_t char_size);
    virtual ~Mapper_003();

protected:

private:
    int char_bank_ = 0;

    virtual uint8_t do_read_prog(uint16_t addr) const final;
    virtual void do_write_prog(uint16_t addr, uint8_t data) final;
    virtual uint8_t do_read_char(uint16_t addr) const final;
    virtual void do_write_char(uint16_t addr, uint8_t data) final;
};

} // namespace

#endif // _H
