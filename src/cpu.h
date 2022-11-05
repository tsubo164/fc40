#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include "apu.h"

namespace nes {

class Cartridge;
class PPU;

struct Instruction {
    uint8_t opcode = 0;
    uint8_t addr_mode = 0;
    uint8_t cycles = 0;
};

struct CpuStatus {
    uint16_t pc = 0;
    uint8_t  a = 0, x = 0, y = 0, p = 0, s = 0;
    uint8_t  lo = 0, hi = 0;
    uint16_t wd = 0;
    uint8_t  code = 0;
    char inst_name[4] = {0};
    char mode_name[4] = {0};
};

class CPU {
public:
    CPU(PPU &ppu) : ppu_(ppu) {}
    ~CPU() {}

private:
    Cartridge *cart_ = nullptr;
    PPU &ppu_;
public:
    APU apu;
private:
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

public:
    void SetCartride(Cartridge *cart);

    void PowerUp();
    void Reset();
    void HandleIRQ();
    void HandleNMI();
    void Clock();

    // DMA
    void InputController(uint8_t controller_id, uint8_t input);
    bool IsSuspended() const;
    void Resume();
    uint8_t GetDmaPage() const;
    uint8_t PeekData(uint16_t addr) const;
    void GetStatus(CpuStatus &stat) const;

    // debug
    void SetPC(uint16_t addr);
    int GetCycles() const;

    // read and write
    void write_byte(uint16_t addr, uint8_t data);
    uint8_t read_byte(uint16_t addr);
    uint16_t read_word(uint16_t addr);
    uint8_t peek_byte(uint16_t addr) const;
    uint16_t peek_word(uint16_t addr) const;
    uint8_t fetch();
    uint16_t fetch_word();
    // address
    uint16_t abs_indirect(uint16_t abs) const;
    uint16_t zp_indirect(uint8_t zp) const;
    uint16_t fetch_address(int mode, int *page_crossed);
    // flags and registers
    void set_pc(uint16_t addr);
    void set_flag(uint8_t flag, uint8_t val);
    uint8_t get_flag(uint8_t flag) const;
    uint8_t update_zn(uint8_t val);
    void set_a(uint8_t val);
    void set_x(uint8_t val);
    void set_y(uint8_t val);
    void set_s(uint8_t val);
    void set_p(uint8_t val);
    // push and pop
    void push(uint8_t val);
    uint8_t pop();
    void push_word(uint16_t val);
    uint16_t pop_word();
    void compare(uint8_t a, uint8_t b);
    bool branch_if(uint16_t addr, bool cond);
    void add_a_m(uint8_t data);
    // instruction
    int execute(Instruction inst);
};

} // namespace

#endif // _H
