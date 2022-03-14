#ifndef CPU_H
#define CPU_H

#include <stdlib.h>
#include <stdint.h>

struct status {
    char carry;
    char zero;
    char interrupt;
    char decimal;
    char brk;
    char reserved;
    char overflow;
    char negative;
};

struct registers {
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t s;
    struct status p;
    uint16_t pc;
};

struct CPU {
    uint8_t *prog;
    size_t prog_size;

    struct registers reg;
    int cycle;
};

extern void reset(struct CPU *cpu);
extern void run(struct CPU *cpu);

extern void execute(struct CPU *cpu);

#endif /* _H */
