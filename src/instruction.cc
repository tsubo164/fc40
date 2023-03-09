#include "instruction.h"
#include <cassert>

namespace nes {

static const uint8_t addr_mode_table[] = {
//       +00  +01  +02  +03  +04  +05  +06  +07  +08  +09  +0A  +0B  +0C  +0D  +0E  +0F
/*0x00*/ IMP, IZX, IMP, IZX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, ACC, IMM, ABS, ABS, ABS, ABS,
/*0x10*/ REL, IZY, IMP, IZY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
/*0x20*/ ABS, IZX, IMP, IZX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, ACC, IMM, ABS, ABS, ABS, ABS,
/*0x30*/ REL, IZY, IMP, IZY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
/*0x40*/ IMP, IZX, IMP, IZX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, ACC, IMM, ABS, ABS, ABS, ABS,
/*0x50*/ REL, IZY, IMP, IZY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
/*0x60*/ IMP, IZX, IMP, IZX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, ACC, IMM, IND, ABS, ABS, ABS,
/*0x70*/ REL, IZY, IMP, IZY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
/*0x80*/ IMM, IZX, IMM, IZX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,
/*0x90*/ REL, IZY, IMP, IZY, ZPX, ZPX, ZPY, ZPY, IMP, ABY, IMP, ABY, ABX, ABX, ABY, ABY,
/*0xA0*/ IMM, IZX, IMM, IZX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,
/*0xB0*/ REL, IZY, IMP, IZY, ZPX, ZPX, ZPY, ZPY, IMP, ABY, IMP, ABY, ABX, ABX, ABY, ABY,
/*0xC0*/ IMM, IZX, IMM, IZX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,
/*0xD0*/ REL, IZY, IMP, IZY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
/*0xE0*/ IMM, IZX, IMM, IZX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,
/*0xF0*/ REL, IZY, IMP, IZY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX
};

static const uint8_t opcode_table[] = {
//      00   01   02   03   04   05   06   07   08   09   0A   0B   0C   0D   0E   0F
/*00*/ BRK, ORA,   0, SLO, NOP, ORA, ASL, SLO, PHP, ORA, ASL,   0, NOP, ORA, ASL, SLO,
/*10*/ BPL, ORA,   0, SLO, NOP, ORA, ASL, SLO, CLC, ORA, NOP, SLO, NOP, ORA, ASL, SLO,
/*20*/ JSR, AND,   0, RLA, BIT, AND, ROL, RLA, PLP, AND, ROL,   0, BIT, AND, ROL, RLA,
/*30*/ BMI, AND,   0, RLA, NOP, AND, ROL, RLA, SEC, AND, NOP, RLA, NOP, AND, ROL, RLA,
/*40*/ RTI, EOR,   0, SRE, NOP, EOR, LSR, SRE, PHA, EOR, LSR,   0, JMP, EOR, LSR, SRE,
/*50*/ BVC, EOR,   0, SRE, NOP, EOR, LSR, SRE, CLI, EOR, NOP, SRE, NOP, EOR, LSR, SRE,
/*60*/ RTS, ADC,   0, RRA, NOP, ADC, ROR, RRA, PLA, ADC, ROR,   0, JMP, ADC, ROR, RRA,
/*70*/ BVS, ADC,   0, RRA, NOP, ADC, ROR, RRA, SEI, ADC, NOP, RRA, NOP, ADC, ROR, RRA,
/*80*/ NOP, STA, NOP, SAX, STY, STA, STX, SAX, DEY, NOP, TXA,   0, STY, STA, STX, SAX,
/*90*/ BCC, STA,   0,   0, STY, STA, STX, SAX, TYA, STA, TXS,   0,   0, STA,   0,   0,
/*A0*/ LDY, LDA, LDX, LAX, LDY, LDA, LDX, LAX, TAY, LDA, TAX, LAX, LDY, LDA, LDX, LAX,
/*B0*/ BCS, LDA,   0, LAX, LDY, LDA, LDX, LAX, CLV, LDA, TSX,   0, LDY, LDA, LDX, LAX,
/*C0*/ CPY, CMP, NOP, DCP, CPY, CMP, DEC, DCP, INY, CMP, DEX,   0, CPY, CMP, DEC, DCP,
/*D0*/ BNE, CMP,   0, DCP, NOP, CMP, DEC, DCP, CLD, CMP, NOP, DCP, NOP, CMP, DEC, DCP,
/*E0*/ CPX, SBC, NOP, ISC, CPX, SBC, INC, ISC, INX, SBC, NOP, SBC, CPX, SBC, INC, ISC,
/*F0*/ BEQ, SBC,   0, ISC, NOP, SBC, INC, ISC, SED, SBC, NOP, ISC, NOP, SBC, INC, ISC
};

static const int8_t cycle_table[] = {
//     00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F
/*00*/  7,  6,  0,  8,  3,  3,  5,  5,  3,  2,  2,  2,  4,  4,  6,  6,
/*10*/  2,  5,  0,  8,  4,  4,  6,  6,  2,  4,  2,  7,  4,  4,  7,  7,
/*20*/  6,  6,  0,  8,  3,  3,  5,  5,  4,  2,  2,  2,  4,  4,  6,  6,
/*30*/  2,  5,  0,  8,  4,  4,  6,  6,  2,  4,  2,  7,  4,  4,  7,  7,
/*40*/  6,  6,  0,  8,  3,  3,  5,  5,  3,  2,  2,  2,  3,  4,  6,  6,
/*50*/  2,  5,  0,  8,  4,  4,  6,  6,  2,  4,  2,  7,  4,  4,  7,  7,
/*60*/  6,  6,  0,  8,  3,  3,  5,  5,  4,  2,  2,  2,  5,  4,  6,  6,
/*70*/  2,  5,  0,  8,  4,  4,  6,  6,  2,  4,  2,  7,  4,  4,  7,  7,
/*80*/  2,  6,  2,  6,  3,  3,  3,  3,  2,  2,  2,  2,  4,  4,  4,  4,
/*90*/  2,  6,  0,  6,  4,  4,  4,  4,  2,  5,  2,  5,  5,  5,  5,  5,
/*A0*/  2,  6,  2,  6,  3,  3,  3,  3,  2,  2,  2,  2,  4,  4,  4,  4,
/*B0*/  2,  5,  0,  5,  4,  4,  4,  4,  2,  4,  2,  4,  4,  4,  4,  4,
/*C0*/  2,  6,  2,  8,  3,  3,  5,  5,  2,  2,  2,  2,  4,  4,  6,  6,
/*D0*/  2,  5,  0,  8,  4,  4,  6,  6,  2,  4,  2,  7,  4,  4,  7,  7,
/*E0*/  2,  6,  2,  8,  3,  3,  5,  5,  2,  2,  2,  2,  4,  4,  6,  6,
/*F0*/  2,  5,  0,  8,  4,  4,  6,  6,  2,  4,  2,  7,  4,  4,  7,  7
};

static uint8_t get_instruction_bytes(int addr_mode)
{
    switch (addr_mode) {
    case ABS: case ABX: case ABY: case IND:
        return 3;

    case IMM: case IZX: case IZY: case ZPG: case ZPX: case ZPY: case REL:
        return 2;

    case ACC: case IMP:
        return 1;

    default:
        assert(!"Unreachable");
        return 0;
    }
}

#define E(enum_) case enum_: return #enum_;
const char *GetAddressingModeName(uint8_t mode)
{
    switch (mode) {
    E(ABS) E(ABX) E(ABY) E(ACC) E(IMM) E(IMP) E(IND) E(IZX)
    E(IZY) E(REL) E(ZPG) E(ZPX) E(ZPY)
    default: return "???";
    }
}

const char *GetOperationName(uint8_t oper)
{
    switch (oper) {
    E(LDA) E(LDX) E(LDY) E(STA) E(STX) E(STY) E(TAX) E(TAY)
    E(TSX) E(TXA) E(TXS) E(TYA) E(PHA) E(PHP) E(PLA) E(PLP)
    E(ASL) E(LSR) E(ROL) E(ROR) E(AND) E(EOR) E(ORA) E(BIT)
    E(ADC) E(SBC) E(CMP) E(CPX) E(CPY) E(INC) E(INX) E(INY)
    E(DEC) E(DEX) E(DEY) E(JMP) E(JSR) E(BRK) E(RTI) E(RTS)
    E(BCC) E(BCS) E(BEQ) E(BMI) E(BNE) E(BPL) E(BVC) E(BVS)
    E(CLC) E(CLD) E(CLI) E(CLV) E(SEC) E(SED) E(SEI) E(LAX)
    E(SAX) E(DCP) E(ISC) E(SLO) E(RLA) E(SRE) E(RRA) E(NOP)
    case ILL:
    default: return "???";
    }
}
#undef E

Instruction Decode(uint8_t code)
{
    Instruction inst;

    inst.operation = opcode_table[code];
    inst.addr_mode = addr_mode_table[code];
    inst.cycles    = cycle_table[code];
    inst.bytes     = get_instruction_bytes(inst.addr_mode);

    return inst;
}

} // namespace
