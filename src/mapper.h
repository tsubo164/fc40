#ifndef MAPPER_H
#define MAPPER_H

#include <stdlib.h>
#include <stdint.h>

struct mapper {
    const uint8_t *prog_rom;
    const uint8_t *char_rom;
    size_t prog_size;
    size_t char_size;

    uint8_t (*read_func)(const struct mapper *m, uint16_t addr);
    void (*write_func)(struct mapper *m, uint16_t addr, uint8_t data);

    void (*init_func)(void);
    void (*finish_func)(void);
};

extern int open_mapper(struct mapper *m, int id, size_t prog_size, size_t char_size);
extern void close_mapper(struct mapper *m);

extern uint8_t read_mapper(const struct mapper *m, uint16_t addr);
extern void write_mapper(struct mapper *m, uint16_t addr, uint8_t data);

#endif /* _H */
