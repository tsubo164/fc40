#ifndef MAPPER_H
#define MAPPER_H

//#include <stdlib.h>
#include <cstdint>

namespace nes {

class Mapper {
public:
    Mapper() {}
    ~Mapper() {}

    const uint8_t *prog_rom;
    const uint8_t *char_rom;
    size_t prog_size;
    size_t char_size;

    uint8_t (*read_func)(const Mapper *m, uint16_t addr);
    void (*write_func)(Mapper *m, uint16_t addr, uint8_t data);

    void (*init_func)(void);
    void (*finish_func)(void);

    uint8_t ReadProg(uint16_t addr) const;
    void WriteProg(uint16_t addr, uint8_t data);
    uint8_t ReadChar(uint16_t addr) const;
    void WriteChar(uint16_t addr, uint8_t data);

private:
    uint8_t do_read_prog(uint16_t addr) const;
    void do_write_prog(uint16_t addr, uint8_t data);
    uint8_t do_read_char(uint16_t addr) const;
    void do_write_char(uint16_t addr, uint8_t data);
};

extern int open_mapper(Mapper *m, int id, size_t prog_size, size_t char_size);
extern void close_mapper(Mapper *m);

} // namespace

#endif // _H
