#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <cstdlib>
#include <cstdint>
#include "mapper.h"

namespace nes {

class Cartridge {
public:
    uint8_t *prog_rom;
    uint8_t *char_rom;
    size_t prog_size;
    size_t char_size;

    uint8_t mirroring;
    uint8_t mapper_id;
    uint8_t nbanks;

    Mapper *mapper = nullptr;
    uint8_t mapper_supported;

    uint8_t ReadProg(uint16_t addr) const;
    void WriteProg(uint16_t addr, uint8_t data);
    uint8_t ReadChar(uint16_t addr) const;
    void WriteChar(uint16_t addr, uint8_t data);
};

extern Cartridge *open_cartridge(const char *filename);
extern void close_cartridge(Cartridge *cart);

extern int is_vertical_mirroring(const Cartridge *cart);

} // namespace

#endif // _H
