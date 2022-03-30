#include <stdio.h>
#include "cpu.h"
#include "ppu.h"
#include "cartridge.h"

static void write_byte(struct CPU *cpu, uint16_t addr, uint8_t data)
{
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        cpu->wram[addr & 0x07FF] = data;
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
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        return cpu->wram[addr & 0x07FF];
    }
    else if (addr == 0x2000) {
    }
    else if (addr == 0x2006) {
    }
    else if (addr == 0x2007) {
    }
    else if (addr >= 0x8000 && addr <= 0xFFFF) {
        return rom_read(cpu->cart, addr);
    }
    return 0;
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

enum addr_mode {ABS, ABX, ABY, ACC, IMM, IMP, IND, IZX, IZY, REL, ZPG, ZPX, ZPY};

static const uint8_t addr_mode_table[] = {
/*       +00  +01  +02  +03  +04  +05  +06  +07  +08  +09  +0A  +0B  +0C  +0D  +0E  +0F */
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

    case ACC:
        /* no address for register */
        return 0;

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
            const uint16_t za = fetch(cpu) + cpu->reg.x;
            const uint16_t lo = read_byte(cpu, za & 0xFF);
            const uint16_t hi = read_byte(cpu, (za + 1) & 0xFF);
            return (hi << 8) | lo;
        }

    case IZY:
        {
            /* addr = {[arg], [arg + 1]} + Y */
            const uint16_t za  = fetch(cpu);
            const uint16_t lo = read_byte(cpu, za & 0xFF);
            const uint16_t hi = read_byte(cpu, (za + 1) & 0xFF);
            const uint16_t addr = (hi << 8) | lo;
            if (is_page_crossing(addr, cpu->reg.y))
                *page_crossed = 1;
            return addr + cpu->reg.y;
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

enum opcode {
    /* undocumented */
    UDC = 0,
    /* load and store */
    LDA, LDX, LDY, STA, STX, STY,
    /* transfer */
    TAX, TAY, TSX, TXA, TXS, TYA,
    /* stack */
    PHA, PHP, PLA, PLP,
    /* shift */
    ASL, LSR, ROL, ROR,
    /* logic */
    AND, EOR, ORA, BIT,
    /* arithmetic */
    ADC, SBC, CMP, CPX, CPY,
    /* increment and decrement */
    INC, INX, INY, DEC, DEX, DEY,
    /* control */
    JMP, JSR, BRK, RTI, RTS,
    /* branch */
    BCC, BCS, BEQ, BMI, BNE, BPL, BVC, BVS,
    /* flag */
    CLC, CLD, CLI, CLV, SEC, SED, SEI,
    /* no op */
    NOP
};

static const uint8_t opcode_table[] = {
/*      00   01   02   03   04   05   06   07   08   09   0A   0B   0C   0D   0E   0F */
/*00*/ BRK, ORA,   0,   0, NOP, ORA, ASL,   0, PHP, ORA, ASL,   0, NOP, ORA, ASL,   0,
/*10*/ BPL, ORA,   0,   0, NOP, ORA, ASL,   0, CLC, ORA, NOP,   0, NOP, ORA, ASL,   0,
/*20*/ JSR, AND,   0,   0, BIT, AND, ROL,   0, PLP, AND, ROL,   0, BIT, AND, ROL,   0,
/*30*/ BMI, AND,   0,   0, NOP, AND, ROL,   0, SEC, AND, NOP,   0, NOP, AND, ROL,   0,
/*40*/ RTI, EOR,   0,   0, NOP, EOR, LSR,   0, PHA, EOR, LSR,   0, JMP, EOR, LSR,   0,
/*50*/ BVC, EOR,   0,   0, NOP, EOR, LSR,   0, CLI, EOR, NOP,   0, NOP, EOR, LSR,   0,
/*60*/ RTS, ADC,   0,   0, NOP, ADC, ROR,   0, PLA, ADC, ROR,   0, JMP, ADC, ROR,   0,
/*70*/ BVS, ADC,   0,   0, NOP, ADC, ROR,   0, SEI, ADC, NOP,   0, NOP, ADC, ROR,   0,
/*80*/ NOP, STA, NOP,   0, STY, STA, STX,   0, DEY, NOP, TXA,   0, STY, STA, STX,   0,
/*90*/ BCC, STA,   0,   0, STY, STA, STX,   0, TYA, STA, TXS,   0,   0, STA,   0,   0,
/*A0*/ LDY, LDA, LDX,   0, LDY, LDA, LDX,   0, TAY, LDA, TAX,   0, LDY, LDA, LDX,   0,
/*B0*/ BCS, LDA,   0,   0, LDY, LDA, LDX,   0, CLV, LDA, TSX,   0, LDY, LDA, LDX,   0,
/*C0*/ CPY, CMP, NOP,   0, CPY, CMP, DEC,   0, INY, CMP, DEX,   0, CPY, CMP, DEC,   0,
/*D0*/ BNE, CMP,   0,   0, NOP, CMP, DEC,   0, CLD, CMP, NOP,   0, NOP, CMP, DEC,   0,
/*E0*/ CPX, SBC, NOP,   0, CPX, SBC, INC,   0, INX, SBC, NOP,   0, CPX, SBC, INC,   0,
/*F0*/ BEQ, SBC,   0,   0, NOP, SBC, INC,   0, SED, SBC, NOP,   0, NOP, SBC, INC,   0
};

static const char opcode_name_table[][4] = {
"BRK","ORA",   "",   "","NOP","ORA","ASL",   "","PHP","ORA","ASL",   "","NOP","ORA","ASL",   "",
"BPL","ORA",   "",   "","NOP","ORA","ASL",   "","CLC","ORA","NOP",   "","NOP","ORA","ASL",   "",
"JSR","AND",   "",   "","BIT","AND","ROL",   "","PLP","AND","ROL",   "","BIT","AND","ROL",   "",
"BMI","AND",   "",   "","NOP","AND","ROL",   "","SEC","AND","NOP",   "","NOP","AND","ROL",   "",
"RTI","EOR",   "",   "","NOP","EOR","LSR",   "","PHA","EOR","LSR",   "","JMP","EOR","LSR",   "",
"BVC","EOR",   "",   "","NOP","EOR","LSR",   "","CLI","EOR","NOP",   "","NOP","EOR","LSR",   "",
"RTS","ADC",   "",   "","NOP","ADC","ROR",   "","PLA","ADC","ROR",   "","JMP","ADC","ROR",   "",
"BVS","ADC",   "",   "","NOP","ADC","ROR",   "","SEI","ADC","NOP",   "","NOP","ADC","ROR",   "",
"NOP","STA","NOP",   "","STY","STA","STX",   "","DEY","NOP","TXA",   "","STY","STA","STX",   "",
"BCC","STA",   "",   "","STY","STA","STX",   "","TYA","STA","TXS",   "",   "","STA",   "",   "",
"LDY","LDA","LDX",   "","LDY","LDA","LDX",   "","TAY","LDA","TAX",   "","LDY","LDA","LDX",   "",
"BCS","LDA",   "",   "","LDY","LDA","LDX",   "","CLV","LDA","TSX",   "","LDY","LDA","LDX",   "",
"CPY","CMP","NOP",   "","CPY","CMP","DEC",   "","INY","CMP","DEX",   "","CPY","CMP","DEC",   "",
"BNE","CMP",   "",   "","NOP","CMP","DEC",   "","CLD","CMP","NOP",   "","NOP","CMP","DEC",   "",
"CPX","SBC","NOP",   "","CPX","SBC","INC",   "","INX","SBC","NOP",   "","CPX","SBC","INC",   "",
"BEQ","SBC",   "",   "","NOP","SBC","INC",   "","SED","SBC","NOP",   "","NOP","SBC","INC",   ""
};

static const int8_t cycle_table[] = {
/*     00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F */
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

static uint8_t update_zn(struct CPU *cpu, uint8_t val)
{
    set_flag(cpu, Z, val == 0x00);
    set_flag(cpu, N, val & 0x80);
    return val;
}

static void set_a(struct CPU *cpu, uint8_t val)
{
    cpu->reg.a = val;
    update_zn(cpu, cpu->reg.a);
}

static void set_x(struct CPU *cpu, uint8_t val)
{
    cpu->reg.x = val;
    update_zn(cpu, cpu->reg.x);
}

static void set_y(struct CPU *cpu, uint8_t val)
{
    cpu->reg.y = val;
    update_zn(cpu, cpu->reg.y);
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
    write_byte(cpu, 0x0100 | cpu->reg.s, val);
    set_s(cpu, cpu->reg.s - 1);
}

static uint8_t pop(struct CPU *cpu)
{
    set_s(cpu, cpu->reg.s + 1);
    return read_byte(cpu, 0x0100 | cpu->reg.s);
}

static void push_word(struct CPU *cpu, uint16_t val)
{
    push(cpu, val >> 8);
    push(cpu, val);
}

static uint16_t pop_word(struct CPU *cpu)
{
    const uint16_t lo = pop(cpu);
    const uint16_t hi = pop(cpu);

    return (hi << 8) | lo;
}

static void compare(struct CPU *cpu, uint8_t a, uint8_t b)
{
    set_flag(cpu, C, a >= b);
    update_zn(cpu, a - b);
}

static int branch_if(struct CPU *cpu, uint16_t addr, int cond)
{
    if (!cond)
        return 0;

    set_pc(cpu, addr);
    return 1;
}

static int is_positive(uint8_t val)
{
    return !(val & 0x80);
}

static void add_a_m(struct CPU *cpu, uint8_t data)
{
    const uint16_t m = data;
    const uint16_t a = cpu->reg.a;
    const uint16_t c = get_flag(cpu, C);
    const uint16_t r = a + m + c;
    const int A = is_positive(a);
    const int M = is_positive(m);
    const int R = is_positive(r);

    set_flag(cpu, C, r > 0xFF);
    set_flag(cpu, V, (A && M && !R) || (!A && !M && R));
    set_a(cpu, r);
}

struct instruction {
    uint8_t opcode;
    uint8_t addr_mode;
    uint8_t cycles;
};

static struct instruction decode(uint8_t code)
{
    struct instruction inst;

    inst.opcode    = opcode_table[code];
    inst.addr_mode = addr_mode_table[code];
    inst.cycles    = cycle_table[code];

    return inst;
}

static int execute(struct CPU *cpu, struct instruction inst)
{
    int page_crossed = 0;
    int branch_taken = 0;
    const uint8_t mode = inst.addr_mode;
    const uint16_t addr = fetch_address(cpu, mode, &page_crossed);

    switch (inst.opcode) {

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

    /* Store Accumulator in Memory: A -> M () */
    case STA:
        write_byte(cpu, addr, cpu->reg.a);
        break;

    /* Store Index Register X in Memory: X -> M () */
    case STX:
        write_byte(cpu, addr, cpu->reg.x);
        break;

    /* Store Index Register Y in Memory: Y -> M () */
    case STY:
        write_byte(cpu, addr, cpu->reg.y);
        break;

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

    /* Push Accumulator on Stack: () */
    case PHA:
        push(cpu, cpu->reg.a);
        break;

    /* Push Processor Status on Stack: () */
    case PHP:
        push(cpu, cpu->reg.p | B);
        break;

    /* Pull Accumulator from Stack: (N, Z) */
    case PLA:
        set_a(cpu, pop(cpu));
        break;

    /* Pull Processor Status from Stack: (N, V, D, I, Z, C) */
    case PLP:
        set_p(cpu, pop(cpu));
        set_flag(cpu, B, 0);
        break;

    /* Arithmetic Shift Left: C <- /M7...M0/ <- 0 (N, Z, C) */
    case ASL:
        if (mode == ACC) {
            const uint8_t data = cpu->reg.a;
            set_flag(cpu, C, data & 0x80);
            set_a(cpu, data << 1);
        } else {
            uint8_t data = read_byte(cpu, addr);
            set_flag(cpu, C, data & 0x80);
            data <<= 1;
            update_zn(cpu, data);
            write_byte(cpu, addr, data);
        }
        break;

    /* Logical Shift Right: 0 -> /M7...M0/ -> C (N, Z, C) */
    case LSR:
        if (mode == ACC) {
            const uint8_t data = cpu->reg.a;
            set_flag(cpu, C, data & 0x01);
            set_a(cpu, data >> 1);
        } else {
            uint8_t data = read_byte(cpu, addr);
            set_flag(cpu, C, data & 0x01);
            data >>= 1;
            update_zn(cpu, data);
            write_byte(cpu, addr, data);
        }
        break;

    /* Rotate Left: C <- /M7...M0/ <- C (N, Z, C) */
    case ROL:
        if (mode == ACC) {
            const uint8_t carry = get_flag(cpu, C);
            const uint8_t data = cpu->reg.a;
            set_flag(cpu, C, data & 0x80);
            set_a(cpu, (data << 1) | carry);
        } else {
            const uint8_t carry = get_flag(cpu, C);
            uint8_t data = read_byte(cpu, addr);
            set_flag(cpu, C, data & 0x80);
            data = (data << 1) | carry;
            update_zn(cpu, data);
            write_byte(cpu, addr, data);
        }
        break;

    /* Rotate Right: C -> /M7...M0/ -> C (N, Z, C) */
    case ROR:
        if (mode == ACC) {
            const uint8_t carry = get_flag(cpu, C);
            const uint8_t data = cpu->reg.a;
            set_flag(cpu, C, data & 0x01);
            set_a(cpu, (data >> 1) | (carry << 7));
        } else {
            const uint8_t carry = get_flag(cpu, C);
            uint8_t data = read_byte(cpu, addr);
            set_flag(cpu, C, data & 0x01);
            data = (data >> 1) | (carry << 7);
            update_zn(cpu, data);
            write_byte(cpu, addr, data);
        }
        break;

    /* AND Memory with Accumulator: A & M -> A (N, Z) */
    case AND:
        set_a(cpu, cpu->reg.a & read_byte(cpu, addr));
        break;

    /* Exclusive OR Memory with Accumulator: A ^ M -> A (N, Z) */
    case EOR:
        set_a(cpu, cpu->reg.a ^ read_byte(cpu, addr));
        break;

    /* OR Memory with Accumulator: A | M -> A (N, Z) */
    case ORA:
        set_a(cpu, cpu->reg.a | read_byte(cpu, addr));
        break;

    /* Test Bits in Memory with Accumulator: A & M (N, V, Z) */
    case BIT:
        {
            const uint8_t data = read_byte(cpu, addr);
            set_flag(cpu, Z, (cpu->reg.a & data) == 0x00);
            set_flag(cpu, N, data & N);
            set_flag(cpu, V, data & V);
        }
        break;

    /* Add Memory to Accumulator with Carry: A + M + C -> A, C (N, V, Z, C) */
    case ADC:
        add_a_m(cpu, read_byte(cpu, addr));
        break;

    /* Subtract Memory to Accumulator with Carry: A - M - ~C -> A (N, V, Z, C) */
    case SBC:
        /* A - M - ~C = A + (-M) - (1 - C)
         *            = A + (-M) - 1 + C
         *            = A + (~M + 1) - 1 + C
         *            = A + (~M) + C */
        add_a_m(cpu, ~read_byte(cpu, addr));
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

    /* Increment Memory by One: M + 1 -> M (N, Z) */
    case INC:
        {
            const uint8_t data = read_byte(cpu, addr) + 1;
            update_zn(cpu, data);
            write_byte(cpu, addr, data);
        }
        break;

    /* Increment Index Register X by One: X + 1 -> X (N, Z) */
    case INX:
        set_x(cpu, cpu->reg.x + 1);
        break;

    /* Increment Index Register Y by One: Y + 1 -> Y (N, Z) */
    case INY:
        set_y(cpu, cpu->reg.y + 1);
        break;

    /* Decrement Memory by One: M - 1 -> M (N, Z) */
    case DEC:
        {
            const uint8_t data = read_byte(cpu, addr) - 1;
            update_zn(cpu, data);
            write_byte(cpu, addr, data);
        }
        break;

    /* Decrement Index Register X by One: X - 1 -> X (N, Z) */
    case DEX:
        set_x(cpu, cpu->reg.x - 1);
        break;

    /* Decrement Index Register Y by One: Y - 1 -> Y (N, Z) */
    case DEY:
        set_y(cpu, cpu->reg.y - 1);
        break;

    /* Jump Indirect: [PC + 1] -> PCL, [PC + 2] -> PCH () */
    case JMP:
        set_pc(cpu, addr);
        break;

    /* Jump to Subroutine: push(PC + 2), [PC + 1] -> PCL, [PC + 2] -> PCH () */
    case JSR:
        /* the last byte of the jump instruction */
        set_pc(cpu, cpu->reg.pc - 1);
        push_word(cpu, cpu->reg.pc);
        set_pc(cpu, addr);
        break;

    /* Break Command: push(PC + 2), [FFFE] -> PCL, [FFFF] ->PCH (I) */
    case BRK:
        /* an extra byte of spacing for a break mark */
        set_pc(cpu, cpu->reg.pc + 1);
        push_word(cpu, cpu->reg.pc);
        push(cpu, cpu->reg.p | B);

        set_flag(cpu, I, 1);
        set_pc(cpu, read_word(cpu, 0xFFFE));
        break;

    /* Return from Interrupt: pop(P) pop(PC) (N, V, D, I, Z, C) */
    case RTI:
        set_p(cpu, pop(cpu));
        set_flag(cpu, B, 0);
        set_pc(cpu, pop_word(cpu));
        break;

    /* Return from Subroutine: pop(PC), PC + 1 -> PC () */
    case RTS:
        set_pc(cpu, pop_word(cpu));
        set_pc(cpu, cpu->reg.pc + 1);
        break;

    /* Branch on Carry Clear: () */
    case BCC:
        branch_taken = branch_if(cpu, addr, get_flag(cpu, C) == 0);
        break;

    /* Branch on Carry Set: () */
    case BCS:
        branch_taken = branch_if(cpu, addr, get_flag(cpu, C) == 1);
        break;

    /* Branch on Result Zero: () */
    case BEQ:
        branch_taken = branch_if(cpu, addr, get_flag(cpu, Z) == 1);
        break;

    /* Branch on Result Minus: () */
    case BMI:
        branch_taken = branch_if(cpu, addr, get_flag(cpu, N) == 1);
        break;

    /* Branch on Result Not Zero: () */
    case BNE:
        branch_taken = branch_if(cpu, addr, get_flag(cpu, Z) == 0);
        break;

    /* Branch on Result Plus: () */
    case BPL:
        branch_taken = branch_if(cpu, addr, get_flag(cpu, N) == 0);
        break;

    /* Branch on Overflow Clear: () */
    case BVC:
        branch_taken = branch_if(cpu, addr, get_flag(cpu, V) == 0);
        break;

    /* Branch on Overflow Set: () */
    case BVS:
        branch_taken = branch_if(cpu, addr, get_flag(cpu, V) == 1);
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

    /* No Operation: () */
    case NOP:
        break;

    default:
        break;
    }

    return inst.cycles + page_crossed + branch_taken;
}

static void print_code(const struct CPU *cpu)
{
    const uint16_t pc = cpu->reg.pc;
    const uint8_t x = cpu->reg.x;
    const uint8_t y = cpu->reg.y;
    uint8_t code, hi, lo;
    struct instruction inst;
    const char *name = "???";
    int N = 48, n = 0;

    code = read_byte(cpu, cpu->reg.pc);
    inst = decode(code);
    name = opcode_name_table[code];

    printf("%04X  %02X %n", pc, code, &n);
    N -= n;

    switch (inst.addr_mode) {

    case IND:
        lo = read_byte(cpu, pc + 1);
        hi = read_byte(cpu, pc + 2);
        printf("%02X %02X  %s ($%04X) = %04X%n",
                lo, hi, name, (hi << 8) | lo, read_word(cpu, (hi << 8) | lo), &n);
        N -= n;
        break;

    case ABS:
        lo = read_byte(cpu, pc + 1);
        hi = read_byte(cpu, pc + 2);
        printf("%02X %02X  %s $%04X%n", lo, hi, name, (hi << 8) | lo, &n);
        N -= n;
        if (inst.opcode != JMP && inst.opcode != JSR) {
            printf(" = %02X%n", read_byte(cpu, (hi << 8) | lo), &n);
            N -= n;
        }
        break;

    case ABX:
        lo = read_byte(cpu, pc + 1);
        hi = read_byte(cpu, pc + 2);
        printf("%02X %02X  %s $%04X,X @ %04X = %02X%n",
                lo, hi, name, (hi << 8) | lo,
                ((hi << 8) | lo) + x,
                read_byte(cpu, ((hi << 8) | lo) + x), &n);
        N -= n;
        break;

    case ABY:
        lo = read_byte(cpu, pc + 1);
        hi = read_byte(cpu, pc + 2);
        printf("%02X %02X  %s $%04X,Y @ %04X = %02X%n",
                lo, hi, name, (hi << 8) | lo,
                ((hi << 8) | lo) + y,
                read_byte(cpu, ((hi << 8) | lo) + y), &n);
        N -= n;
        break;

    case IZX:
        lo = read_byte(cpu, pc + 1);
        {
            const uint16_t l = read_byte(cpu, (lo + x) & 0xFF);
            const uint16_t h = read_byte(cpu, (lo + x + 1) & 0xFF);
            printf("%02X     %s ($%02X,X) @ %02X = %04X = %02X%n",
                    lo, name, lo,
                    (lo + x) & 0xFF,
                    (h << 8) | l,
                    read_byte(cpu, (h << 8) | l), &n);
        }
        N -= n;
        break;

    case IZY:
        lo = read_byte(cpu, pc + 1);
        {
            const uint16_t l = read_byte(cpu, (lo) & 0xFF);
            const uint16_t h = read_byte(cpu, (lo + 1) & 0xFF);
            const uint16_t w = ((h << 8) | l) + y;
            printf("%02X     %s ($%02X),Y = %04X @ %04X = %02X%n",
                    lo, name, lo,
                    (h << 8) | l, w,
                    read_byte(cpu, w), &n);
        }
        N -= n;
        break;

    case REL:
        lo = read_byte(cpu, pc + 1);
        printf("%02X     %s $%04X%n", lo, name, (pc + 2) + (int8_t)lo, &n);
        N -= n;
        break;

    case IMM:
        lo = read_byte(cpu, pc + 1);
        printf("%02X     %s #$%02X%n", lo, name, lo, &n);
        N -= n;
        break;

    case ZPG:
        lo = read_byte(cpu, pc + 1);
        printf("%02X     %s $%02X = %02X %n", lo, name, lo, read_byte(cpu, lo), &n);
        N -= n;
        break;

    case ACC:
        lo = read_byte(cpu, pc + 1);
        printf("       %s A%n", name, &n);
        N -= n;
        break;

    case IMP:
        printf("       %s%n", name, &n);
        N -= n;
        break;

        /*
    case ZPX: break;
    case ZPY: break;
        */
    default: break;
    }

    printf("%*s", N, " ");
    printf("A:%02X X:%02X Y:%02X P:%02X SP:%02X",
            cpu->reg.a, cpu->reg.x, cpu->reg.y, cpu->reg.p, cpu->reg.s);

    printf("\n");
}

void reset(struct CPU *cpu)
{
    const uint16_t addr = read_word(cpu, 0xFFFC);
    set_pc(cpu, addr);

    /* registers */
    set_a(cpu, 0x00);
    set_x(cpu, 0x00);
    set_y(cpu, 0x00);
    set_s(cpu, 0xFD);
    set_p(cpu, 0x00 | I);

    /* takes cycles */
    cpu->cycles = 8;
}

void clock_cpu(struct CPU *cpu)
{
    if (cpu->cycles == 0) {
        uint8_t code, cycs;
        struct instruction inst;

        if (cpu->log_mode) {
            print_code(cpu);
            cpu->log_line++;
        }

        code = fetch(cpu);
        inst = decode(code);
        cycs = execute(cpu, inst);

        cpu->cycles = cycs;
    }

    cpu->cycles--;
}
