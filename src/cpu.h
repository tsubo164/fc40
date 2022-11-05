#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include "apu.h"

namespace nes {

class Cartridge;
class PPU;

struct CPU {
    Cartridge *cart = nullptr;
    PPU *ppu;
    APU apu;
    int cycles = 0;
    int suspended = 0;

    // registers
    uint8_t a = 0;
    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t s = 0; // stack pointer
    uint8_t p = 0; // processor status
    uint16_t pc = 0;

    // 4 2KB ram. 3 of them are mirroring
    uint8_t wram[2048] = {0};

    uint8_t controller_input[2] = {0};
    uint8_t controller_state[2] = {0};

    // dma
    uint8_t dma_page = 0;

    void PowerUp();
    void Reset();
    void HandleIRQ();
    void HandleNMI();
    void Clock();

    void write_byte(uint16_t addr, uint8_t data);
    uint8_t read_byte(uint16_t addr);
    uint16_t read_word(uint16_t addr);
    uint8_t peek_byte(uint16_t addr) const;
    uint16_t peek_word(uint16_t addr) const;
    uint8_t fetch();
    uint16_t fetch_word();
};

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

} // namespace

#endif // _H
