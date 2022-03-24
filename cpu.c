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

static uint8_t read_byte(const struct CPU *cpu, uint16_t addr)
{
    return cpu->prog[addr - 0x8000];
}

static uint16_t read_word(const struct CPU *cpu, uint16_t addr)
{
    const uint16_t lo = read_byte(cpu, addr);
    const uint16_t hi = read_byte(cpu, addr + 1);

    return (hi << 8) | lo;
}

static uint8_t fetch(struct CPU *cpu)
{
    return read_byte(cpu, cpu->reg.pc++);
}

static uint16_t fetch_word(struct CPU *cpu)
{
    const uint16_t lo = fetch(cpu);
    const uint16_t hi = fetch(cpu);

    return (hi << 8) | lo;
}

static uint8_t is_page_crossing(uint16_t addr, uint8_t addend)
{
    return (addr & 0x00FF) + (addend & 0x00FF) > 0x00FF;
}

enum addr_mode {ABS, ABX, ABY, IMM, IMP, IND, IZX, IZY, REL, ZPG, ZPX, ZPY};

static const uint8_t addr_mode_table[] = {
/*       +00  +01  +02  +03  +04  +05  +06  +07  +08  +09  +0A  +0B  +0C  +0D  +0E  +0F */
/*0x00*/ IMP, IZX, IMP, IZX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,
/*0x10*/ REL, IZY, IMP, IZY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
/*0x20*/ ABS, IZX, IMP, IZX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,
/*0x30*/ REL, IZY, IMP, IZY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
/*0x40*/ IMP, IZX, IMP, IZX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,
/*0x50*/ REL, IZY, IMP, IZY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
/*0x60*/ IMP, IZX, IMP, IZX, ZPG, ZPG, ZPG, ZPG, IMP, IMM, IMP, IMM, IND, ABS, ABS, ABS,
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

static uint16_t fetch_address(struct CPU *cpu, int mode, int *page_crossed)
{
    *page_crossed = 0;

    switch (mode) {

    case ABS:
        return fetch_word(cpu);

    case ABX:
        {
            /* addr = arg + X */
            const uint16_t addr = fetch_word(cpu);
            if (is_page_crossing(addr, cpu->reg.x))
                *page_crossed = 1;
            return addr + cpu->reg.x;
        }

    case ABY:
        {
            /* addr = arg + Y */
            const uint16_t addr = fetch_word(cpu);
            if (is_page_crossing(addr, cpu->reg.y))
                *page_crossed = 1;
            return addr + cpu->reg.y;
        }

    case IMM:
        /* address where the immediate value is stored */
        return cpu->reg.pc++;

    case IMP:
        /* no address */
        return 0;

    case IND:
        {
            const uint16_t addr = fetch_word(cpu);
            if ((addr & 0x00FF) == 0x00FF)
                /* emulate page boundary hardware bug */
                return (read_byte(cpu, addr & 0xFF00) << 8) | read_byte(cpu, addr);
            else
                /* normal behavior */
                return read_word(cpu, addr);
        }

    case IZX:
        {
            /* addr = {[arg + X], [arg + X + 1]} */
            const uint16_t addr = read_word(cpu, fetch(cpu) + cpu->reg.x);
            return read_word(cpu, addr);
        }

    case IZY:
        {
            /* addr = {[arg], [arg + 1]} + Y */
            const uint16_t addr = read_word(cpu, fetch(cpu));
            if (is_page_crossing(addr, cpu->reg.y))
                *page_crossed = 1;
            return read_word(cpu, addr + cpu->reg.y);
        }

    case REL:
        {
            /* fetch data first, then add it to the pc */
            const uint8_t offset = fetch(cpu);
            return cpu->reg.pc + (int8_t) offset;
        }

    case ZPG:
        return fetch(cpu);

    case ZPX:
        return (fetch(cpu) + cpu->reg.x) & 0x00FF;

    case ZPY:
        return (fetch(cpu) + cpu->reg.y) & 0x00FF;

    default:
        return 0;
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

static int get_cycle(uint8_t code, int page_crossed)
{
    const int cyc = cycle_table[code];

    if (cyc == 0)
        /* illegal op */
        return 0;

    if (cyc < 0)
        /* add 1 cycle if page boundary is crossed */
        return -1 * cyc + page_crossed;

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
    case IND: break;
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

static void set_pc(struct CPU *cpu, uint16_t addr)
{
    cpu->reg.pc = addr;
}

static void set_flag(struct CPU *cpu, uint8_t flag, uint8_t val)
{
    if (val)
        cpu->reg.p |= flag;
    else
        cpu->reg.p &= ~flag;
}

static uint8_t get_flag(const struct CPU *cpu, uint8_t flag)
{
    return (cpu->reg.p & flag) > 0;
}

static void set_a(struct CPU *cpu, uint8_t val)
{
    cpu->reg.a = val;
    set_flag(cpu, Z, cpu->reg.a == 0x00);
    set_flag(cpu, N, cpu->reg.a & 0x80);
}

static void set_x(struct CPU *cpu, uint8_t val)
{
    cpu->reg.x = val;
    set_flag(cpu, Z, cpu->reg.x == 0x00);
    set_flag(cpu, N, cpu->reg.x & 0x80);
}

static void set_y(struct CPU *cpu, uint8_t val)
{
    cpu->reg.y = val;
    set_flag(cpu, Z, cpu->reg.y == 0x00);
    set_flag(cpu, N, cpu->reg.y & 0x80);
}

static void set_s(struct CPU *cpu, uint8_t val)
{
    cpu->reg.s = val;
}

static void set_p(struct CPU *cpu, uint8_t val)
{
    cpu->reg.p = val | U;
}

static void push(struct CPU *cpu, uint8_t val)
{
    write_byte(0x0100 | cpu->reg.s, val);
    set_s(cpu, cpu->reg.s - 1);
}

static uint8_t pop(struct CPU *cpu)
{
    set_s(cpu, cpu->reg.s + 1);
    return read_byte(cpu, 0x0100 | cpu->reg.s);
}

static void compare(struct CPU *cpu, uint8_t a, uint8_t b)
{
    const uint8_t diff = a - b;

    set_flag(cpu, C, diff >= 0x00);
    set_flag(cpu, Z, diff == 0x00);
    set_flag(cpu, N, diff & 0x80);
}

static int branch_on(struct CPU *cpu, uint16_t addr, int test)
{
    if (!test)
        return 0;

    set_pc(cpu, addr);
    return 1;
}

static void execute(struct CPU *cpu)
{
    const uint16_t inst_addr = cpu->reg.pc;
    const uint8_t code = fetch(cpu);

    const uint8_t mode = addr_mode_table[code];
    const uint8_t opecode = opecode_table[code];
    int page_crossed = 0;
    int branch_taken = 0;
    const uint16_t addr = fetch_address(cpu, mode, &page_crossed);;
    int cycle = get_cycle(code, page_crossed);

    switch (opecode) {
    case ADC: break;
    /* XXX doesn't exist */
    case AHX: break;
    /* XXX doesn't exist */
    case ALR: break;
    /* XXX doesn't exist */
    case ANC: break;

    /* AND Memory with Accumulator: A & M -> A (N, Z) */
    case AND:
        set_a(cpu, cpu->reg.a & read_byte(cpu, addr));
        break;

    /* XXX doesn't exist */
    case ARR: break;

    /* Arithmetic Shift Left: C <- /M7...M0/ <- 0 (N, Z) */
    case ASL: break;

    /* XXX doesn't exist */
    case AXS: break;

    /* Branch on Carry Clear: () */
    case BCC:
        if (get_flag(cpu, C) == 0) {
            set_pc(cpu, addr);
            cycle++;
        }
        break;

    /* Branch on Carry Set: () */
    case BCS:
        if (get_flag(cpu, C) == 1) {
            set_pc(cpu, addr);
            cycle++;
        }
        break;

    /* Branch on Result Zero: () */
    case BEQ:
        if (get_flag(cpu, Z) == 1) {
            set_pc(cpu, addr);
            cycle++;
        }
        break;

    /* Test Bits in Memory with Accumulator: A & M (N, V, Z) */
    case BIT:
        {
            const uint8_t data = read_byte(cpu, addr);
            set_flag(cpu, Z, (cpu->reg.a & data) == 0x00);
            set_flag(cpu, N, data & (1 << 7));
            set_flag(cpu, V, data & (1 << 6));
        }
        break;

    /* Branch on Result Minus: () */
    case BMI:
        if (get_flag(cpu, N) == 1) {
            set_pc(cpu, addr);
            cycle++;
        }
        break;

    /* Branch on Result Not Zero: () */
    case BNE:
        if (0)
            branch_taken = branch_on(cpu, addr, get_flag(cpu, Z) == 0);
        if (get_flag(cpu, Z) == 0) {
            set_pc(cpu, addr);
            cycle++;
        }
        break;

    /* Branch on Result Minus: () */
    case BPL:
        if (get_flag(cpu, N) == 0) {
            set_pc(cpu, addr);
            cycle++;
        }
        break;

    case BRK:
        break;

    /* Branch on Overflow Clear: () */
    case BVC:
        if (get_flag(cpu, V) == 0) {
            set_pc(cpu, addr);
            cycle++;
        }
        break;

    /* Branch on Overflow Set: () */
    case BVS:
        if (get_flag(cpu, V) == 1) {
            set_pc(cpu, addr);
            cycle++;
        }
        break;

    /* Clear Carry Flag: 0 -> C (C) */
    case CLC:
        set_flag(cpu, C, 0);
        break;

    /* Clear Decimal Mode: 0 -> D (D) */
    case CLD:
        set_flag(cpu, D, 0);
        break;

    /* Clear Interrupt Disable: 0 -> I (I) */
    case CLI:
        set_flag(cpu, I, 0);
        break;

    /* Clear Overflow Flag: 0 -> V (V) */
    case CLV:
        set_flag(cpu, V, 0);
        break;

    /* Compare Memory and Accumulator: A - M (N, Z, C) */
    case CMP:
        compare(cpu, cpu->reg.a, read_byte(cpu, addr));
        break;

    /* Compare Index Register X to Memory: X - M (N, Z, C) */
    case CPX:
        compare(cpu, cpu->reg.x, read_byte(cpu, addr));
        break;

    /* Compare Index Register Y to Memory: Y - M (N, Z, C) */
    case CPY:
        compare(cpu, cpu->reg.y, read_byte(cpu, addr));
        break;

    /* XXX doesn't exist */
    case DCP: break;
    case DEC: break;

    /* Decrement Index Register X by One: X - 1 -> X (N, Z) */
    case DEX:
        set_x(cpu, cpu->reg.x - 1);
        break;

    /* Decrement Index Register Y by One: Y - 1 -> Y (N, Z) */
    case DEY:
        set_y(cpu, cpu->reg.y - 1);
        break;

    /* Exclusive OR Memory with Accumulator: A ^ M -> A (N, Z) */
    case EOR:
        set_a(cpu, cpu->reg.a ^ read_byte(cpu, addr));
        break;

    case INC: break;

    /* Increment Index Register X by One: X + 1 -> X (N, Z) */
    case INX:
        set_x(cpu, cpu->reg.x + 1);
        break;

    /* Increment Index Register Y by One: Y + 1 -> Y (N, Z) */
    case INY:
        set_y(cpu, cpu->reg.y + 1);
        break;

    /* XXX doesn't exist */
    case ISC: break;

    /* Jump Indirect: PC = {[PC+1],[PC+2]} () */
    case JMP:
        set_pc(cpu, addr);
        break;

    case JSR: break;
    /* XXX doesn't exist */
    case LAS: break;
    /* XXX doesn't exist */
    case LAX: break;

    /* Load Accumulator with Memory: M -> A (N, Z) */
    case LDA:
        set_a(cpu, read_byte(cpu, addr));
        break;

    /* Load Index Register X from Memory: M -> X (N, Z) */
    case LDX:
        set_x(cpu, read_byte(cpu, addr));
        break;

    /* Load Index Register Y from Memory: M -> Y (N, Z) */
    case LDY:
        set_y(cpu, read_byte(cpu, addr));
        break;

    case LSR: break;

    /* No Operation: () */
    case NOP:
        break;

    /* OR Memory with Accumulator: A | M -> A (N, Z) */
    case ORA:
        set_a(cpu, cpu->reg.a | read_byte(cpu, addr));
        break;

    /* Push Accumulator on Stack: () */
    case PHA:
        push(cpu, cpu->reg.a);
        break;

    /* Push Processor Status on Stack: () */
    case PHP:
        push(cpu, cpu->reg.p);
        break;

    /* Pull Accumulator from Stack: (N, Z) */
    case PLA:
        set_a(cpu, pop(cpu));
        break;

    /* Pull Processor Status from Stack: (N, V, D, I, Z, C) */
    case PLP:
        set_p(cpu, pop(cpu));
        break;

    /* XXX doesn't exist */
    case RLA: break;
    case ROL: break;
    case ROR: break;
    /* XXX doesn't exist */
    case RRA: break;
    case RTI: break;
    case RTS: break;
    /* XXX doesn't exist */
    case SAX: break;
    case SBC: break;

    /* Set Carry Flag: 1 -> C (C) */
    case SEC:
        set_flag(cpu, C, 1);
        break;

    /* Set Decimal Mode: 1 -> D (D) */
    case SED:
        set_flag(cpu, D, 1);
        break;

    /* Set Interrupt Disable: 1 -> I (I) */
    case SEI:
        set_flag(cpu, I, 1);
        break;

    /* XXX doesn't exist */
    case SHX: break;
    /* XXX doesn't exist */
    case SHY: break;

    /* XXX doesn't exist */
    case SLO: break;

    /* XXX doesn't exist */
    case SRE: break;

    /* Store Accumulator in Memory: A -> M () */
    case STA:
        write_byte(addr, cpu->reg.a);
        break;

    /* XXX doesn't exist */
    case STP: break;

    /* Store Index Register X in Memory: X -> M () */
    case STX:
        write_byte(addr, cpu->reg.x);
        break;

    /* Store Index Register Y in Memory: Y -> M () */
    case STY:
        write_byte(addr, cpu->reg.y);
        break;

    /* XXX doesn't exist */
    case TAS: break;

    /* Transfer Accumulator to Index X: A -> X (N, Z) */
    case TAX:
        set_x(cpu, cpu->reg.a);
        break;

    /* Transfer Accumulator to Index Y: A -> Y (N, Z) */
    case TAY:
        set_y(cpu, cpu->reg.a);
        break;

    /* Transfer Stack Pointer to Index X: S -> X (N, Z) */
    case TSX:
        set_x(cpu, cpu->reg.s);
        break;

    /* Transfer Index X to Accumulator: X -> A (N, Z) */
    case TXA:
        set_a(cpu, cpu->reg.x);
        break;

    /* Transfer Index X to Stack Pointer: X -> S () */
    case TXS:
        set_s(cpu, cpu->reg.x);
        break;

    /* Transfer Index Y to Accumulator: Y -> A (N, Z) */
    case TYA:
        set_a(cpu, cpu->reg.y);
        break;

    /* XXX doesn't exist */
    case XAA: break;

    default:
        break;
    }

    cpu->cycle = cycle;

    if (0)
        print_code(inst_addr, code, mode, addr);
}

void reset(struct CPU *cpu)
{
    uint16_t addr;

    addr = read_word(cpu, 0xFFFC);
    set_pc(cpu, addr);

    /* registers */
    set_a(cpu, 0x00);
    set_x(cpu, 0x00);
    set_y(cpu, 0x00);
    set_s(cpu, 0xFD);
    set_p(cpu, 0x00);

    /* takes cycles */
    cpu->cycle = 8;
}

void clock_cpu(struct CPU *cpu)
{
    if (cpu->cycle == 0)
        execute(cpu);

    cpu->cycle--;
}
