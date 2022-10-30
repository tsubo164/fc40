#ifndef MAPPER_H
#define MAPPER_H

#include <stdlib.h>
#include <stdint.h>

struct mapper {
    int32_t (*map_prog_addr_func)(uint16_t addr);
    int32_t (*map_char_addr_func)(uint16_t addr);
    void (*write_func)(uint16_t addr, uint8_t data);

    void (*init_func)(void);
    void (*finish_func)(void);
};

extern int open_mapper(struct mapper *m, int id, size_t prog_size, size_t char_size);
extern void close_mapper(struct mapper *m);

extern int32_t map_prog_addr(const struct mapper *m, uint16_t addr);
extern int32_t map_char_addr(const struct mapper *m, uint16_t addr);
extern void write_mapper(const struct mapper *m, uint16_t addr, uint8_t data);

#endif /* _H */
