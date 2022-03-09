#include <stdio.h>
#include "cpu.h"

static void jump(struct CPU *cpu, uint16_t addr)
{
    cpu->reg.pc = addr;
}

void adc(struct CPU *cpu, uint16_t operand) {}
void ahx(struct CPU *cpu, uint16_t operand) {}
void alr(struct CPU *cpu, uint16_t operand) {}
void anc(struct CPU *cpu, uint16_t operand) {}
void and(struct CPU *cpu, uint16_t operand) {}
void arr(struct CPU *cpu, uint16_t operand) {}
void asl(struct CPU *cpu, uint16_t operand) {}
void axs(struct CPU *cpu, uint16_t operand) {}
void bcc(struct CPU *cpu, uint16_t operand) {}
void bcs(struct CPU *cpu, uint16_t operand) {}
void beq(struct CPU *cpu, uint16_t operand) {}
void bit(struct CPU *cpu, uint16_t operand) {}
void bmi(struct CPU *cpu, uint16_t operand) {}

//void bne(uint16_t operand)
void bne(struct CPU *cpu, uint16_t operand)
{
    if (cpu->reg.p.zero == 0)
        jump(cpu, operand);
}

void bpl(struct CPU *cpu, uint16_t operand) {}
void brk(struct CPU *cpu, uint16_t operand) {}
void bvc(struct CPU *cpu, uint16_t operand) {}
void bvs(struct CPU *cpu, uint16_t operand) {}
void clc(struct CPU *cpu, uint16_t operand) {}
void cld(struct CPU *cpu, uint16_t operand) {}
void cli(struct CPU *cpu, uint16_t operand) {}
void clv(struct CPU *cpu, uint16_t operand) {}
void cmp(struct CPU *cpu, uint16_t operand) {}
void cpx(struct CPU *cpu, uint16_t operand) {}
void cpy(struct CPU *cpu, uint16_t operand) {}
void dcp(struct CPU *cpu, uint16_t operand) {}
void dec(struct CPU *cpu, uint16_t operand) {}
void dex(struct CPU *cpu, uint16_t operand) {}
void dey(struct CPU *cpu, uint16_t operand) {}
void eor(struct CPU *cpu, uint16_t operand) {}
void inc(struct CPU *cpu, uint16_t operand) {}
void inx(struct CPU *cpu, uint16_t operand) {}
void iny(struct CPU *cpu, uint16_t operand) {}
void isc(struct CPU *cpu, uint16_t operand) {}
void jmp(struct CPU *cpu, uint16_t operand) {}
void jsr(struct CPU *cpu, uint16_t operand) {}
void las(struct CPU *cpu, uint16_t operand) {}
void lax(struct CPU *cpu, uint16_t operand) {}
void lda(struct CPU *cpu, uint16_t operand) {}
void ldx(struct CPU *cpu, uint16_t operand) {}
void ldy(struct CPU *cpu, uint16_t operand) {}
void lsr(struct CPU *cpu, uint16_t operand) {}
void nop(struct CPU *cpu, uint16_t operand) {}
void ora(struct CPU *cpu, uint16_t operand) {}
void pha(struct CPU *cpu, uint16_t operand) {}
void php(struct CPU *cpu, uint16_t operand) {}
void pla(struct CPU *cpu, uint16_t operand) {}
void plp(struct CPU *cpu, uint16_t operand) {}
void rla(struct CPU *cpu, uint16_t operand) {}
void rol(struct CPU *cpu, uint16_t operand) {}
void ror(struct CPU *cpu, uint16_t operand) {}
void rra(struct CPU *cpu, uint16_t operand) {}
void rti(struct CPU *cpu, uint16_t operand) {}
void rts(struct CPU *cpu, uint16_t operand) {}
void sax(struct CPU *cpu, uint16_t operand) {}
void sbc(struct CPU *cpu, uint16_t operand) {}
void sec(struct CPU *cpu, uint16_t operand) {}
void sed(struct CPU *cpu, uint16_t operand) {}
void sei(struct CPU *cpu, uint16_t operand) {}
void shx(struct CPU *cpu, uint16_t operand) {}
void shy(struct CPU *cpu, uint16_t operand) {}
void slo(struct CPU *cpu, uint16_t operand) {}
void sre(struct CPU *cpu, uint16_t operand) {}
void sta(struct CPU *cpu, uint16_t operand) {}
void stp(struct CPU *cpu, uint16_t operand) {}
void stx(struct CPU *cpu, uint16_t operand) {}
void sty(struct CPU *cpu, uint16_t operand) {}
void tas(struct CPU *cpu, uint16_t operand) {}
void tax(struct CPU *cpu, uint16_t operand) {}
void tay(struct CPU *cpu, uint16_t operand) {}
void tsx(struct CPU *cpu, uint16_t operand) {}
void txa(struct CPU *cpu, uint16_t operand) {}
void txs(struct CPU *cpu, uint16_t operand) {}
void tya(struct CPU *cpu, uint16_t operand) {}
void xaa(struct CPU *cpu, uint16_t operand) {}

typedef void (*opecode_fn)(struct CPU *, uint16_t);

opecode_fn opecode_table_[] = {
/*       +00  +01  +02  +03  +04  +05  +06  +07  +08  +09  +0A  +0B  +0C  +0D  +0E  +0F */
/*0x00*/ brk, ora, stp, slo, nop, ora, asl, slo, php, ora, asl, anc, nop, ora, asl, slo,
/*0x10*/ bpl, ora, stp, slo, nop, ora, asl, slo, clc, ora, nop, slo, nop, ora, asl, slo,
/*0x20*/ jsr, and, stp, rla, bit, and, rol, rla, plp, and, rol, anc, bit, and, rol, rla,
/*0x30*/ bmi, and, stp, rla, nop, and, rol, rla, sec, and, nop, rla, nop, and, rol, rla,
/*0x40*/ rti, eor, stp, sre, nop, eor, lsr, sre, pha, eor, lsr, alr, jmp, eor, lsr, sre,
/*0x50*/ bvc, eor, stp, sre, nop, eor, lsr, sre, cli, eor, nop, sre, nop, eor, lsr, sre,
/*0x60*/ rts, adc, stp, rra, nop, adc, ror, rra, pla, adc, ror, arr, jmp, adc, ror, rra,
/*0x70*/ bvs, adc, stp, rra, nop, adc, ror, rra, sei, adc, nop, rra, nop, adc, ror, rra,
/*0x80*/ nop, sta, nop, sax, sty, sta, stx, sax, dey, nop, txa, xaa, sty, sta, stx, sax,
/*0x90*/ bcc, sta, stp, ahx, sty, sta, stx, sax, tya, sta, txs, tas, shy, sta, shx, ahx,
/*0xA0*/ ldy, lda, ldx, lax, ldy, lda, ldx, lax, tay, lda, tax, lax, ldy, lda, ldx, lax,
/*0xB0*/ bcs, lda, stp, lax, ldy, lda, ldx, lax, clv, lda, tsx, las, ldy, lda, ldx, lax,
/*0xC0*/ cpy, cmp, nop, dcp, cpy, cmp, dec, dcp, iny, cmp, dex, axs, cpy, cmp, dec, dcp,
/*0xD0*/ bne, cmp, stp, dcp, nop, cmp, dec, dcp, cld, cmp, nop, dcp, nop, cmp, dec, dcp,
/*0xE0*/ cpx, sbc, nop, isc, cpx, sbc, inc, isc, inx, sbc, nop, sbc, cpx, sbc, inc, isc,
/*0xF0*/ beq, sbc, stp, isc, nop, sbc, inc, isc, sed, sbc, nop, isc, nop, sbc, inc, isc
};

#if 0
uint16_t abs(void) { return 0; }
uint16_t abx(void) { return 0; }
uint16_t aby(void) { return 0; }
uint16_t imm(void) { return 0; }
uint16_t imp(void) { return 0; }
uint16_t ina(void) { return 0; }
uint16_t izx(void) { return 0; }
uint16_t izy(void) { return 0; }
uint16_t rel(void) { return 0; }
uint16_t zpg(void) { return 0; }
uint16_t zpx(void) { return 0; }
uint16_t zpy(void) { return 0; }

typedef uint16_t (*operand)(void);

static operand_fn operand_table[] = {
/*       +00  +01  +02  +03  +04  +05  +06  +07  +08  +09  +0A  +0B  +0C  +0D  +0E  +0F */
/*0x00*/ imp, izx, imp, izx, zpg, zpg, zpg, zpg, imp, imm, imp, imm, abs, abs, abs, abs,
/*0x10*/ rel, izy, imp, izy, zpx, zpx, zpx, zpx, imp, aby, imp, aby, abx, abx, abx, abx
/*0x20*/ abs, izx, imp, izx, zpg, zpg, zpg, zpg, imp, imm, imp, imm, abs, abs, abs, abs,
/*0x30*/ rel, izy, imp, izy, zpx, zpx, zpx, zpx, imp, aby, imp, aby, abx, abx, abx, abx
/*0x40*/ imp, izx, imp, izx, zpg, zpg, zpg, zpg, imp, imm, imp, imm, abs, abs, abs, abs,
/*0x50*/ rel, izy, imp, izy, zpx, zpx, zpx, zpx, imp, aby, imp, aby, abx, abx, abx, abx
/*0x60*/ imp, izx, imp, izx, zpg, zpg, zpg, zpg, imp, imm, imp, imm, ina, abs, abs, abs,
/*0x70*/ rel, izy, imp, izy, zpx, zpx, zpx, zpx, imp, aby, imp, aby, abx, abx, abx, abx
/*0x80*/ imm, izx, imm, izx, zpg, zpg, zpg, zpg, imp, imm, imp, imm, abs, abs, abs, abs,
/*0x90*/ rel, izy, imp, izy, zpx, zpx, zpy, zpy, imp, aby, imp, aby, abx, abx, aby, aby
/*0xA0*/ imm, izx, imm, izx, zpg, zpg, zpg, zpg, imp, imm, imp, imm, abs, abs, abs, abs,
/*0xB0*/ rel, izy, imp, izy, zpx, zpx, zpy, zpy, imp, aby, imp, aby, abx, abx, aby, aby
/*0xC0*/ imm, izx, imm, izx, zpg, zpg, zpg, zpg, imp, imm, imp, imm, abs, abs, abs, abs,
/*0xD0*/ rel, izy, imp, izy, zpx, zpx, zpx, zpx, imp, aby, imp, aby, abx, abx, abx, abx
/*0xE0*/ imm, izx, imm, izx, zpg, zpg, zpg, zpg, imp, imm, imp, imm, abs, abs, abs, abs,
/*0xF0*/ rel, izy, imp, izy, zpx, zpx, zpx, zpx, imp, aby, imp, aby, abx, abx, abx, abx
};
#endif

static uint8_t read_byte(struct CPU *cpu, uint16_t addr)
{
    return cpu->prog[addr - 0x8000];
}

/*
static uint16_t read_word(struct CPU *cpu, uint16_t addr)
{
    uint16_t lo, hi;

    lo = read_byte(cpu, addr);
    hi = read_byte(cpu, addr + 1);

    return (hi << 8) + lo;
}
*/

uint8_t fetch(struct CPU *cpu)
{
    return read_byte(cpu, cpu->reg.pc++);
}

enum addr_mode {ABS, ABX, ABY, IMM, IMP, INA, IZX, IZY, REL, ZPG, ZPX, ZPY};

static enum addr_mode addr_mode_table[] = {
/*       +00  +01  +02  +03  +04  +05  +06  +07  +08  +09  +0A  +0B  +0C  +0D  +0E  +0F */
/*0x00*/ IMP, IZX, IMP, IZX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,
/*0x10*/ REL, IZY, IMP, IZY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
/*0x20*/ ABS, IZX, IMP, IZX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,
/*0x30*/ REL, IZY, IMP, IZY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
/*0x40*/ IMP, IZX, IMP, IZX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,
/*0x50*/ REL, IZY, IMP, IZY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
/*0x60*/ IMP, IZX, IMP, IZX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, IMP, IMM, INA, ABS, ABS, ABS,
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

uint16_t fetch_operand(struct CPU *cpu, int mode)
{
    switch (mode) {
    case ABS: return 0;
    case ABX: return 0;
    case ABY: return 0;
    case IMM:
        return fetch(cpu);
    case IMP: return 0;
    case INA: return 0;
    case IZX: return 0;
    case IZY: return 0;
    case REL: return 0;
    case ZPG: return 0;
    case ZPX: return 0;
    case ZPY: return 0;
    default: return 0;
    }
}

enum opecode {
    ADC, AHX, ALR, ANC, AND, ARR, ASL, AXS, BCC, BCS, BEQ, BIT, BMI, BNE, BPL, BRK,
    BVC, BVS, CLC, CLD, CLI, CLV, CMP, CPX, CPY, DCP, DEC, DEX, DEY, EOR, INC, INX,
    INY, ISC, JMP, JSR, LAS, LAX, LDA, LDX, LDY, LSR, NOP, ORA, PHA, PHP, PLA, PLP,
    RLA, ROL, ROR, RRA, RTI, RTS, SAX, SBC, SEC, SED, SEI, SHX, SHY, SLO, SRE, STA,
    STP, STX, STY, TAS, TAX, TAY, TSX, TXA, TXS, TYA, XAA
};

static const enum opecode opecode_table[] = {
/*       +00  +01  +02  +03  +04  +05  +06  +07  +08  +09  +0A  +0B  +0C  +0D  +0E  +0F */
/*0x00*/ BRK, ORA, STP, SLO, NOP, ORA, ASL, SLO, PHP, ORA, ASL, ANC, NOP, ORA, ASL, SLO,
/*0x10*/ BPL, ORA, STP, SLO, NOP, ORA, ASL, SLO, CLC, ORA, NOP, SLO, NOP, ORA, ASL, SLO,
/*0x20*/ JSR, AND, STP, RLA, BIT, AND, ROL, RLA, PLP, AND, ROL, ANC, BIT, AND, ROL, RLA,
/*0x30*/ BMI, AND, STP, RLA, NOP, AND, ROL, RLA, SEC, AND, NOP, RLA, NOP, AND, ROL, RLA,
/*0x40*/ RTI, EOR, STP, SRE, NOP, EOR, LSR, SRE, PHA, EOR, LSR, ALR, JMP, EOR, LSR, SRE,
/*0x50*/ BVC, EOR, STP, SRE, NOP, EOR, LSR, SRE, CLI, EOR, NOP, SRE, NOP, EOR, LSR, SRE,
/*0x60*/ RTS, ADC, STP, RRA, NOP, ADC, ROR, RRA, PLA, ADC, ROR, ARR, JMP, ADC, ROR, RRA,
/*0x70*/ BVS, ADC, STP, RRA, NOP, ADC, ROR, RRA, SEI, ADC, NOP, RRA, NOP, ADC, ROR, RRA,
/*0x80*/ NOP, STA, NOP, SAX, STY, STA, STX, SAX, DEY, NOP, TXA, XAA, STY, STA, STX, SAX,
/*0x90*/ BCC, STA, STP, AHX, STY, STA, STX, SAX, TYA, STA, TXS, TAS, SHY, STA, SHX, AHX,
/*0xA0*/ LDY, LDA, LDX, LAX, LDY, LDA, LDX, LAX, TAY, LDA, TAX, LAX, LDY, LDA, LDX, LAX,
/*0xB0*/ BCS, LDA, STP, LAX, LDY, LDA, LDX, LAX, CLV, LDA, TSX, LAS, LDY, LDA, LDX, LAX,
/*0xC0*/ CPY, CMP, NOP, DCP, CPY, CMP, DEC, DCP, INY, CMP, DEX, AXS, CPY, CMP, DEC, DCP,
/*0xD0*/ BNE, CMP, STP, DCP, NOP, CMP, DEC, DCP, CLD, CMP, NOP, DCP, NOP, CMP, DEC, DCP,
/*0xE0*/ CPX, SBC, NOP, ISC, CPX, SBC, INC, ISC, INX, SBC, NOP, SBC, CPX, SBC, INC, ISC,
/*0xF0*/ BEQ, SBC, STP, ISC, NOP, SBC, INC, ISC, SED, SBC, NOP, ISC, NOP, SBC, INC, ISC
};

static const char opecode_name_table[][4] = {
/*       +00  +01  +02  +03  +04  +05  +06  +07  +08  +09  +0A  +0B  +0C  +0D  +0E  +0F */
/*0x00*/ "BRK", "ORA", "STP", "SLO", "NOP", "ORA", "ASL", "SLO", "PHP", "ORA", "ASL", "ANC", "NOP", "ORA", "ASL", "SLO",
/*0x10*/ "BPL", "ORA", "STP", "SLO", "NOP", "ORA", "ASL", "SLO", "CLC", "ORA", "NOP", "SLO", "NOP", "ORA", "ASL", "SLO",
/*0x20*/ "JSR", "AND", "STP", "RLA", "BIT", "AND", "ROL", "RLA", "PLP", "AND", "ROL", "ANC", "BIT", "AND", "ROL", "RLA",
/*0x30*/ "BMI", "AND", "STP", "RLA", "NOP", "AND", "ROL", "RLA", "SEC", "AND", "NOP", "RLA", "NOP", "AND", "ROL", "RLA",
/*0x40*/ "RTI", "EOR", "STP", "SRE", "NOP", "EOR", "LSR", "SRE", "PHA", "EOR", "LSR", "ALR", "JMP", "EOR", "LSR", "SRE",
/*0x50*/ "BVC", "EOR", "STP", "SRE", "NOP", "EOR", "LSR", "SRE", "CLI", "EOR", "NOP", "SRE", "NOP", "EOR", "LSR", "SRE",
/*0x60*/ "RTS", "ADC", "STP", "RRA", "NOP", "ADC", "ROR", "RRA", "PLA", "ADC", "ROR", "ARR", "JMP", "ADC", "ROR", "RRA",
/*0x70*/ "BVS", "ADC", "STP", "RRA", "NOP", "ADC", "ROR", "RRA", "SEI", "ADC", "NOP", "RRA", "NOP", "ADC", "ROR", "RRA",
/*0x80*/ "NOP", "STA", "NOP", "SAX", "STY", "STA", "STX", "SAX", "DEY", "NOP", "TXA", "XAA", "STY", "STA", "STX", "SAX",
/*0x90*/ "BCC", "STA", "STP", "AHX", "STY", "STA", "STX", "SAX", "TYA", "STA", "TXS", "TAS", "SHY", "STA", "SHX", "AHX",
/*0xA0*/ "LDY", "LDA", "LDX", "LAX", "LDY", "LDA", "LDX", "LAX", "TAY", "LDA", "TAX", "LAX", "LDY", "LDA", "LDX", "LAX",
/*0xB0*/ "BCS", "LDA", "STP", "LAX", "LDY", "LDA", "LDX", "LAX", "CLV", "LDA", "TSX", "LAS", "LDY", "LDA", "LDX", "LAX",
/*0xC0*/ "CPY", "CMP", "NOP", "DCP", "CPY", "CMP", "DEC", "DCP", "INY", "CMP", "DEX", "AXS", "CPY", "CMP", "DEC", "DCP",
/*0xD0*/ "BNE", "CMP", "STP", "DCP", "NOP", "CMP", "DEC", "DCP", "CLD", "CMP", "NOP", "DCP", "NOP", "CMP", "DEC", "DCP",
/*0xE0*/ "CPX", "SBC", "NOP", "ISC", "CPX", "SBC", "INC", "ISC", "INX", "SBC", "NOP", "SBC", "CPX", "SBC", "INC", "ISC",
/*0xF0*/ "BEQ", "SBC", "STP", "ISC", "NOP", "SBC", "INC", "ISC", "SED", "SBC", "NOP", "ISC", "NOP", "SBC", "INC", "ISC"
};

void exec(struct CPU *cpu, uint8_t code)
{
    const uint16_t addr = cpu->reg.pc - 1;

    const uint8_t mode = addr_mode_table[code];
    const uint8_t  opecode = opecode_table[code];
    const uint16_t operand = fetch_operand(cpu, mode);;

    switch (opecode) {
    case ADC: break;
    case AHX: break;
    case ALR: break;
    case ANC: break;
    case AND: break;
    case ARR: break;
    case ASL: break;
    case AXS: break;
    case BCC: break;
    case BCS: break;
    case BEQ: break;
    case BIT: break;
    case BMI: break;
    case BNE: break;
    case BPL: break;
    case BRK: break;
    case BVC: break;
    case BVS: break;
    case CLC: break;
    case CLD: break;
    case CLI: break;
    case CLV: break;
    case CMP: break;
    case CPX: break;
    case CPY: break;
    case DCP: break;
    case DEC: break;
    case DEX: break;
    case DEY: break;
    case EOR: break;
    case INC: break;
    case INX: break;
    case INY: break;
    case ISC: break;
    case JMP: break;
    case JSR: break;
    case LAS: break;
    case LAX: break;
    case LDA: break;
    case LDX:
        cpu->reg.x = operand;
        break;
    case LDY:
        cpu->reg.y = operand;
        break;
    case LSR: break;
    case NOP:
        break;
    case ORA: break;
    case PHA: break;
    case PHP: break;
    case PLA: break;
    case PLP: break;
    case RLA: break;
    case ROL: break;
    case ROR: break;
    case RRA: break;
    case RTI: break;
    case RTS: break;
    case SAX: break;
    case SBC: break;
    case SEC: break;
    case SED: break;
    case SEI:
        cpu->reg.p.interrupt = 1;
        break;
    case SHX: break;
    case SHY: break;
    case SLO: break;
    case SRE: break;
    case STA: break;
    case STP: break;
    case STX: break;
    case STY: break;
    case TAS: break;
    case TAX: break;
    case TAY: break;
    case TSX: break;
    case TXA: break;
    case TXS: break;
    case TYA: break;
    case XAA: break;
    default: break;
    }

    printf("[0x%04X] %s", addr, opecode_name_table[code]);
    switch (mode) {
    case ABS: break;
    case ABX: break;
    case ABY: break;
    case IMM: printf(" #$%02X", operand); break;;
    case IMP: break;
    case INA: break;
    case IZX: break;
    case IZY: break;
    case REL: break;
    case ZPG: break;
    case ZPX: break;
    case ZPY: break;
    default: break;
    }
    printf("\n");
}
