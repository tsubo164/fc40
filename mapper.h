#ifndef MAPPER_H
#define MAPPER_H

#include <stdlib.h>
#include <stdint.h>

struct mapper {
    int (*map_prog_addr_func)(uint16_t addr, uint32_t *mapped);
    int (*map_char_addr_func)(uint16_t addr, uint32_t *mapped);

    void (*init_func)(void);
    void (*finish_func)(void);
};

extern int open_mapper(struct mapper *m, int id, size_t prog_size, size_t char_size);
extern void close_mapper(struct mapper *m);

extern int map_prog_addr(const struct mapper *m, uint16_t addr, uint32_t *mapped);
extern int map_char_addr(const struct mapper *m, uint16_t addr, uint32_t *mapped);

#endif /* _H */
