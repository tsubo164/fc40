#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <cstdint>

namespace nes {

enum AddressingMode {
    ABS, ABX, ABY, ACC, IMM, IMP, IND, IZX, IZY, REL, ZPG, ZPX, ZPY
};

enum Operation {
    // Illegal
    ILL = 0,
    LDA, LDX, LDY, STA, STX, STY, TAX, TAY, TSX, TXA, TXS, TYA, PHA, PHP, PLA, PLP,
    ASL, LSR, ROL, ROR, AND, EOR, ORA, BIT, ADC, SBC, CMP, CPX, CPY, INC, INX, INY,
    DEC, DEX, DEY, JMP, JSR, BRK, RTI, RTS, BCC, BCS, BEQ, BMI, BNE, BPL, BVC, BVS,
    CLC, CLD, CLI, CLV, SEC, SED, SEI,
    // Undocumented. there are actually some more ops.
    LAX, SAX, DCP, ISC, SLO, RLA, SRE, RRA,
    // No op
    NOP
};

struct Instruction {
    uint8_t operation = 0;
    uint8_t addr_mode = 0;
    uint8_t cycles = 0;
    uint8_t bytes = 0;
};

const char *GetAddressingModeName(uint8_t mode);
const char *GetOperationName(uint8_t oper);

Instruction Decode(uint8_t opcode);

} // namespace

#endif // _H
