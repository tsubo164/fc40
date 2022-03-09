#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stdint.h>

struct cartridge {
    uint8_t *prog_rom;
    uint8_t *char_rom;
    size_t prog_size;
    size_t char_size;
};

extern struct cartridge *open_cartridge(const char *filename);
extern void close_cartridge(struct cartridge *cart);

#endif /* _H */
