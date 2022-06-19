#ifndef CPU_H
#define CPU_H

#include <stdint.h>

struct cartridge;
struct PPU;

struct CPU {
    struct cartridge *cart;
    struct PPU *ppu;
    int cycles;
    int suspended;

    /* registers */
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t s; /* stack pointer */
    uint8_t p; /* processor status */
    uint16_t pc;

    /* 4 2KB ram. 3 of them are mirroring */
    uint8_t wram[2048];

    uint8_t controller_input[2];
    uint8_t controller_state[2];

    /* dma */
    uint8_t dma_page;
};

extern void power_up_cpu(struct CPU *cpu);
extern void reset(struct CPU *cpu);
extern void nmi(struct CPU *cpu);
extern void clock_cpu(struct CPU *cpu);

extern void set_controller_input(struct CPU *cpu, uint8_t id, uint8_t input);
extern int is_suspended(const struct CPU *cpu);
extern void resume(struct CPU *cpu);
extern uint8_t get_dma_page(const struct CPU *cpu);

extern uint8_t read_cpu_data(const struct CPU *cpu, uint16_t addr);

struct cpu_status {
    uint16_t pc;
    uint8_t  a, x, y, p, s;
    uint8_t  lo, hi;
    uint16_t wd;
    uint8_t  code;
    char inst_name[4];
    char mode_name[4];
};

extern void get_cpu_status(const struct CPU *cpu, struct cpu_status *stat);
extern uint8_t peek_cpu_data(const struct CPU *cpu, uint16_t addr);

#endif /* _H */
