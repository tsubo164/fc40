#ifndef CPU_H
#define CPU_H

#include <stdlib.h>
#include <stdint.h>

struct cartridge;
struct PPU;

struct registers {
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t s; /* stack pointer */
    uint8_t p; /* processor status */
    uint16_t pc;
};

struct CPU {
    struct cartridge *cart;
    struct PPU *ppu;

    /* 4 2KB ram. 3 of them are mirroring */
    uint8_t wram[2048];
    struct registers reg;
    int cycles;

    uint8_t log_mode;
    uint16_t log_line;
};

extern void reset(struct CPU *cpu);
extern void nmi(struct CPU *cpu);
extern void clock_cpu(struct CPU *cpu);

#endif /* _H */
