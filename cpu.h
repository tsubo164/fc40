#ifndef CPU_H
#define CPU_H

#include <stdlib.h>
#include <stdint.h>

enum status_flag {
    C = 1 << 0, /* carry */
    Z = 1 << 1, /* zero */
    I = 1 << 2, /* disable interrupts */
    D = 1 << 3, /* decimal mode */
    B = 1 << 4, /* break */
    U = 1 << 5, /* unused */
    V = 1 << 6, /* overflow */
    N = 1 << 7  /* negative */
};

struct registers {
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t s; /* stack pointer */
    uint8_t p; /* processor status */
    uint16_t pc;
};

struct CPU {
    uint8_t *prog;
    size_t prog_size;

    struct registers reg;
    int cycles;
};

extern void reset(struct CPU *cpu);
extern void clock_cpu(struct CPU *cpu);

#endif /* _H */
