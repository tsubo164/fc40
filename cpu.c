#include <stdio.h>
#include "cpu.h"
#include "ppu.h"

static void write_byte(uint16_t addr, uint8_t data)
{
    if (addr <= 0x07FF) {
        /* WRAM */
    }
    else if (addr <= 0x1FFF) {
        /* WRAM mirror */
    }
    else if (addr == 0x2000) {
    }
    else if (addr == 0x2006) {
        write_ppu_addr(data);
    }
    else if (addr == 0x2007) {
        write_ppu_data(data);
    }
    else if (addr <= 0x3FFF) {
        /* PPU registers mirror */
    }
}

static uint8_t read_byte(struct CPU *cpu, uint16_t addr)
{
    return cpu->prog[addr - 0x8000];
}

static uint16_t read_word(struct CPU *cpu, uint16_t addr)
{
    uint16_t lo, hi;

    lo = read_byte(cpu, addr);
    hi = read_byte(cpu, addr + 1);

    return (hi << 8) | lo;
}

static uint8_t fetch(struct CPU *cpu)
{
    return read_byte(cpu, cpu->reg.pc++);
}

static uint16_t fetch_word(struct CPU *cpu)
{
    uint16_t lo, hi;

    lo = fetch(cpu);
    hi = fetch(cpu);

    return (hi << 8) | lo;
}

static void jump(struct CPU *cpu, uint16_t addr)
{
    cpu->reg.pc = addr;
}

enum addr_mode {ABS, ABX, ABY, IMM, IMP, INA, IZX, IZY, REL, ZPG, ZPX, ZPY};

static const uint8_t addr_mode_table[] = {
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

static uint16_t fetch_operand(struct CPU *cpu, int mode)
{
    switch (mode) {
    case ABS: return fetch_word(cpu);
    case ABX: return read_byte(cpu, fetch_word(cpu) + cpu->reg.x);
    case ABY: return 0;
    case IMM: return fetch(cpu);
    case IMP: return 0;
    case INA: return 0;
    case IZX: return 0;
    case IZY: return 0;
    case REL:
        {
            /* fetch data first, then add it to the pc */
            uint8_t offset = fetch(cpu);
            return cpu->reg.pc + (int8_t)offset;
        }
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

static const uint8_t opecode_table[] = {
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
"BRK","ORA","STP","SLO","NOP","ORA","ASL","SLO","PHP","ORA","ASL","ANC","NOP","ORA","ASL","SLO",
"BPL","ORA","STP","SLO","NOP","ORA","ASL","SLO","CLC","ORA","NOP","SLO","NOP","ORA","ASL","SLO",
"JSR","AND","STP","RLA","BIT","AND","ROL","RLA","PLP","AND","ROL","ANC","BIT","AND","ROL","RLA",
"BMI","AND","STP","RLA","NOP","AND","ROL","RLA","SEC","AND","NOP","RLA","NOP","AND","ROL","RLA",
"RTI","EOR","STP","SRE","NOP","EOR","LSR","SRE","PHA","EOR","LSR","ALR","JMP","EOR","LSR","SRE",
"BVC","EOR","STP","SRE","NOP","EOR","LSR","SRE","CLI","EOR","NOP","SRE","NOP","EOR","LSR","SRE",
"RTS","ADC","STP","RRA","NOP","ADC","ROR","RRA","PLA","ADC","ROR","ARR","JMP","ADC","ROR","RRA",
"BVS","ADC","STP","RRA","NOP","ADC","ROR","RRA","SEI","ADC","NOP","RRA","NOP","ADC","ROR","RRA",
"NOP","STA","NOP","SAX","STY","STA","STX","SAX","DEY","NOP","TXA","XAA","STY","STA","STX","SAX",
"BCC","STA","STP","AHX","STY","STA","STX","SAX","TYA","STA","TXS","TAS","SHY","STA","SHX","AHX",
"LDY","LDA","LDX","LAX","LDY","LDA","LDX","LAX","TAY","LDA","TAX","LAX","LDY","LDA","LDX","LAX",
"BCS","LDA","STP","LAX","LDY","LDA","LDX","LAX","CLV","LDA","TSX","LAS","LDY","LDA","LDX","LAX",
"CPY","CMP","NOP","DCP","CPY","CMP","DEC","DCP","INY","CMP","DEX","AXS","CPY","CMP","DEC","DCP",
"BNE","CMP","STP","DCP","NOP","CMP","DEC","DCP","CLD","CMP","NOP","DCP","NOP","CMP","DEC","DCP",
"CPX","SBC","NOP","ISC","CPX","SBC","INC","ISC","INX","SBC","NOP","SBC","CPX","SBC","INC","ISC",
"BEQ","SBC","STP","ISC","NOP","SBC","INC","ISC","SED","SBC","NOP","ISC","NOP","SBC","INC","ISC"
};

static const int8_t cycle_table[] = {
/*      +00 +01 +02 +03 +04 +05 +06 +07 +08 +09 +0A +0B +0C +0D +0E +0F */
/*0x00*/  7,  6,  0,  8,  3,  3,  5,  5,  3,  2,  2,  2,  4,  4,  6,  6,
/*0x10*/ -2, -5,  0,  8,  4,  4,  6,  6,  2, -4,  2,  7, -4, -4,  7,  7,
/*0x20*/  6,  6,  0,  8,  3,  3,  5,  5,  4,  2,  2,  2,  4,  4,  6,  6,
/*0x30*/ -2, -5,  0,  8,  4,  4,  6,  6,  2, -4,  2,  7, -4, -4,  7,  7,
/*0x40*/  6,  6,  0,  8,  3,  3,  5,  5,  3,  2,  2,  2,  3,  4,  6,  6,
/*0x50*/ -2, -5,  0,  8,  4,  4,  6,  6,  2, -4,  2,  7, -4, -4,  7,  7,
/*0x60*/  6,  6,  0,  8,  3,  3,  5,  5,  4,  2,  2,  2,  5,  4,  6,  6,
/*0x70*/ -2, -5,  0,  8,  4,  4,  6,  6,  2, -4,  2,  7, -4, -4,  7,  7,
/*0x80*/  2,  6,  2,  6,  3,  3,  3,  3,  2,  2,  2,  2,  4,  4,  4,  4,
/*0x90*/ -2,  6,  0,  6,  4,  4,  4,  4,  2,  5,  2,  5,  5,  5,  5,  5,
/*0xA0*/  2,  6,  2,  6,  3,  3,  3,  3,  2,  2,  2,  2,  4,  4,  4,  4,
/*0xB0*/ -2, -5,  0, -5,  4,  4,  4,  4,  2, -4,  2, -4, -4, -4, -4, -4,
/*0xC0*/  2,  6,  2,  8,  3,  3,  5,  5,  2,  2,  2,  2,  4,  4,  6,  6,
/*0xD0*/ -2, -5,  0,  8,  4,  4,  6,  6,  2, -4,  2,  7, -4, -4,  7,  7,
/*0xE0*/  2,  6,  2,  8,  3,  3,  5,  5,  2,  2,  2,  2,  4,  4,  6,  6,
/*0xF0*/ -2, -5,  0,  8,  4,  4,  6,  6,  2, -4,  2,  7, -4, -4,  7,  7
};

static int get_cycle(uint8_t code)
{
    int cyc = cycle_table[code];

    if (cyc == 0) {
        /* illegal op */
        return 0;
    }

    if (cyc < 0) {
        /* add 1 cycle if page boundary is crossed */
        return -1 * cyc;
    }

    return cyc;
}

static void print_code(uint16_t addr, uint8_t code, uint8_t mode, uint16_t operand)
{
    printf("[0x%04X] %s", addr, opecode_name_table[code]);
    switch (mode) {
    case ABS: printf(" $%04X", operand); break;
    case ABX: printf(" $%02X", operand); break;
    case ABY: break;
    case IMM: printf(" #$%02X", operand); break;
    case IMP: break;
    case INA: break;
    case IZX: break;
    case IZY: break;
    case REL: printf(" $%02X", operand); break;
    case ZPG: break;
    case ZPX: break;
    case ZPY: break;
    default: break;
    }
    printf("\n");
}

static void execute(struct CPU *cpu)
{
    const uint16_t addr = cpu->reg.pc;
    const uint8_t code = fetch(cpu);

    const uint8_t mode = addr_mode_table[code];
    const uint8_t  opecode = opecode_table[code];
    const uint16_t operand = fetch_operand(cpu, mode);;
    const int cycle = get_cycle(code);

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
    case BNE:
        if (cpu->reg.p.zero == 0)
            jump(cpu, operand);
        break;
    case BPL: break;
    case BRK:
        break;
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
    case DEY:
        cpu->reg.y--;
        cpu->reg.p.zero = (cpu->reg.y == 0);
        break;
    case EOR: break;
    case INC: break;
    case INX:
        cpu->reg.x++;
        cpu->reg.p.zero = (cpu->reg.x == 0);
        break;
    case INY: break;
    case ISC: break;
    case JMP:
        jump(cpu, operand);
        break;
    case JSR: break;
    case LAS: break;
    case LAX: break;
    case LDA:
        cpu->reg.a = operand;
        break;
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
    case STA:
        write_byte(operand, cpu->reg.a);
        break;
    case STP: break;
    case STX: break;
    case STY: break;
    case TAS: break;
    case TAX: break;
    case TAY: break;
    case TSX: break;
    case TXA: break;
    case TXS:
        cpu->reg.s = cpu->reg.x + 0x0100;
        break;
    case TYA: break;
    case XAA: break;
    default: break;
    }

    cpu->cycle = cycle;

    if (0)
        print_code(addr, code, mode, operand);
}

void reset(struct CPU *cpu)
{
    const struct status ini = {0};
    uint16_t addr;

    addr = read_word(cpu, 0xFFFC);
    jump(cpu, addr);

    /* registers */
    cpu->reg.a = 0;
    cpu->reg.x = 0;
    cpu->reg.y = 0;
    cpu->reg.s = 0xFD;
    cpu->reg.p = ini;

    /* takes cycles */
    cpu->cycle = 8;
}

void clock_cpu(struct CPU *cpu)
{
    if (cpu->cycle == 0)
        execute(cpu);

    cpu->cycle--;
}
