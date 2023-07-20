#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include "instruction.h"

namespace nes {

class Cartridge;
class PPU;
class APU;

struct CpuStatus {
    uint16_t pc = 0;
    uint8_t  a = 0, x = 0, y = 0, p = 0, s = 0;
};

class CPU {
public:
    CPU(PPU &ppu, APU &apu);
    ~CPU();

    void SetCartride(Cartridge *cart);

    // clock
    void PowerUp();
    void Reset();
    int Run();

    // DMA
    void InputController(uint8_t controller_id, uint8_t input);
    bool IsSuspended() const;
    void Resume();
    uint8_t PeekData(uint16_t addr) const;

    // debug
    CpuStatus GetStatus() const;
    void SetPC(uint16_t addr);
    uint16_t GetPC() const;
    uint16_t GetAbsoluteIndirect(uint16_t abs) const;
    uint16_t GetZeroPageIndirect(uint8_t zp) const;

    void SetTestMode();
    uint64_t GetTotalCycles() const;

private:
    PPU &ppu_;
    APU &apu_;
    Cartridge *cart_ = nullptr;

    uint64_t total_cycles_ = 0;
    bool suspended_ = false;
    bool test_mode_ = false;

    // registers
    uint8_t a_ = 0;
    uint8_t x_ = 0;
    uint8_t y_ = 0;
    uint8_t s_ = 0; // stack pointer
    uint8_t p_ = 0; // processor status
    uint16_t pc_ = 0;

    uint8_t controller_input_[2] = {0};
    uint8_t controller_state_[2] = {0};

    // 4 2KB rams. 3 of them are mirroring
    uint8_t wram_[2048] = {0};

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
    uint16_t fetch_address(int mode, bool *page_crossed);
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
    int execute_instruction();
    // interrupts
    int do_interrupt(uint16_t vector);
    int handle_interrupt();
};

} // namespace

#endif // _H
