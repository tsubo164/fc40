#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stdlib.h>
#include <stdint.h>
#include "mapper.h"

struct cartridge {
    uint8_t *prog_rom;
    uint8_t *char_rom;
    size_t prog_size;
    size_t char_size;

    uint8_t mirroring;
    uint8_t mapper_id;
    uint8_t nbanks;

    struct mapper mapper;
    uint8_t mapper_supported;
};

extern struct cartridge *open_cartridge(const char *filename);
extern void close_cartridge(struct cartridge *cart);

extern uint8_t read_prog_rom(const struct cartridge *cart, uint16_t addr);
extern uint8_t read_char_rom(const struct cartridge *cart, uint16_t addr);
extern void write_cartridge(const struct cartridge *cart, uint16_t addr, uint8_t data);

extern int is_vertical_mirroring(const struct cartridge *cart);

#endif /* _H */
