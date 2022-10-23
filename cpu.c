#include <stdio.h>
#include <string.h>
#include "cpu.h"
#include "ppu.h"
#include "cartridge.h"

enum status_flag {
    C = 1 << 0, /* carry */
    Z = 1 << 1, /* zero */
    I = 1 << 2, /* disable interrupts */
    D = 1 << 3, /* decimal mode */
    B = 1 << 4, /* break */
    U = 1 << 5, /* unused */
    V = 1 << 6, /* overflow */
    N = 1 << 7  /* negative */
};

static void write_byte(struct CPU *cpu, uint16_t addr, uint8_t data)
{
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        cpu->wram[addr & 0x07FF] = data;
    }
    else if (addr == 0x2000) {
        write_ppu_control(cpu->ppu, data);
    }
    else if (addr == 0x2001) {
        write_ppu_mask(cpu->ppu, data);
    }
    else if (addr == 0x2002) {
        /* PPU status not writable */
    }
    else if (addr == 0x2003) {
        write_oam_address(cpu->ppu, data);
    }
    else if (addr == 0x2004) {
        write_oam_data(cpu->ppu, data);
    }
    else if (addr == 0x2005) {
        write_ppu_scroll(cpu->ppu, data);
    }
    else if (addr == 0x2006) {
        write_ppu_address(cpu->ppu, data);
    }
    else if (addr == 0x2007) {
        write_ppu_data(cpu->ppu, data);
    }
    else if (addr >= 0x2008 && addr <= 0x3FFF) {
        /* PPU register mirrored every 8 */
        write_byte(cpu, 0x2000 | (addr & 0x007), data);
    }
    else if (addr == 0x4000) {
        write_apu_square1_volume(&cpu->apu, data);
    }
    else if (addr == 0x4001) {
        write_apu_square1_sweep(&cpu->apu, data);
    }
    else if (addr == 0x4002) {
        write_apu_square1_lo(&cpu->apu, data);
    }
    else if (addr == 0x4003) {
        write_apu_square1_hi(&cpu->apu, data);
    }
    else if (addr == 0x4004) {
        write_apu_square2_volume(&cpu->apu, data);
    }
    else if (addr == 0x4005) {
        write_apu_square2_sweep(&cpu->apu, data);
    }
    else if (addr == 0x4006) {
        write_apu_square2_lo(&cpu->apu, data);
    }
    else if (addr == 0x4007) {
        write_apu_square2_hi(&cpu->apu, data);
    }
    else if (addr == 0x4008) {
        write_apu_triangle_linear(&cpu->apu, data);
    }
    else if (addr == 0x400A) {
        write_apu_triangle_lo(&cpu->apu, data);
    }
    else if (addr == 0x400B) {
        write_apu_triangle_hi(&cpu->apu, data);
    }
    else if (addr == 0x400C) {
        write_apu_noise_volume(&cpu->apu, data);
    }
    else if (addr == 0x400E) {
        write_apu_noise_lo(&cpu->apu, data);
    }
    else if (addr == 0x400F) {
        write_apu_noise_hi(&cpu->apu, data);
    }
    else if (addr == 0x4014) {
        /* DMA */
        cpu->suspended = 1;
        cpu->dma_page = data;
    }
    else if (addr == 0x4015) {
        write_apu_status(&cpu->apu, data);
    }
    else if (addr == 0x4016) {
        const int id = addr & 0x001;
        cpu->controller_state[id] = cpu->controller_input[id];
    }
    else if (addr == 0x4017) {
        write_apu_frame_counter(&cpu->apu, data);
    }
}

static uint8_t read_byte(struct CPU *cpu, uint16_t addr)
{
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        return cpu->wram[addr & 0x07FF];
    }
    else if (addr == 0x2000) {
        /* PPU control not readable */
    }
    else if (addr == 0x2001) {
        /* PPU mask not readable */
    }
    else if (addr == 0x2002) {
        return read_ppu_status(cpu->ppu);
    }
    else if (addr == 0x2003) {
        /* PPU oam address not readable */
    }
    else if (addr == 0x2004) {
        return read_oam_data(cpu->ppu);
    }
    else if (addr == 0x2005) {
        /* PPU scroll not readable */
    }
    else if (addr == 0x2006) {
        /* PPU address not readable */
    }
    else if (addr == 0x2007) {
        return read_ppu_data(cpu->ppu);
    }
    else if (addr >= 0x2008 && addr <= 0x3FFF) {
        /* PPU register mirrored every 8 */
        return read_byte(cpu, 0x2000 | (addr & 0x007));
    }
    else if (addr >= 0x4016 && addr <= 0x4017) {
        const int id = addr & 0x001;
        const uint8_t data = (cpu->controller_state[id] & 0x80) > 0;
        cpu->controller_state[id] <<= 1;
        return data;
    }
    else if (addr >= 0x8000 && addr <= 0xFFFF) {
        return rom_read(cpu->cart, addr);
    }
    return 0;
}

static uint8_t peek_byte(const struct CPU *cpu, uint16_t addr)
{
    if (addr == 0x2002) {
        return peek_ppu_status(cpu->ppu);
    }
    else if (addr >= 0x4016 && addr <= 0x4017) {
        const int id = addr & 0x001;
        return (cpu->controller_state[id] & 0x80) > 0;
    }

    return read_byte((struct CPU *) cpu, addr);
}

static uint16_t peek_word(const struct CPU *cpu, uint16_t addr)
{
    const uint16_t lo = peek_byte(cpu, addr);
    const uint16_t hi = peek_byte(cpu, addr + 1);

    return (hi << 8) | lo;
}

static uint16_t read_word(struct CPU *cpu, uint16_t addr)
{
    const uint16_t lo = read_byte(cpu, addr);
    const uint16_t hi = read_byte(cpu, addr + 1);

    return (hi << 8) | lo;
}

static uint8_t fetch(struct CPU *cpu)
{
    return read_byte(cpu, cpu->pc++);
}

static uint16_t fetch_word(struct CPU *cpu)
{
    const uint16_t lo = fetch(cpu);
    const uint16_t hi = fetch(cpu);

    return (hi << 8) | lo;
}

enum addr_mode {ABS, ABX, ABY, ACC, IMM, IMP, IND, IZX, IZY, REL, ZPG, ZPX, ZPY};

static const char addr_mode_name_table[][4] = {
    "ABS","ABX","ABY","ACC","IMM","IMP","IND","IZX","IZY","REL","ZPG","ZPX","ZPY"
};

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

static uint8_t is_page_crossing(uint16_t addr, uint8_t addend)
{
    return (addr & 0x00FF) + (addend & 0x00FF) > 0x00FF;
}

static uint16_t abs_index(uint16_t abs, uint8_t idx, int *page_crossed)
{
    *page_crossed = is_page_crossing(abs, idx);
    return abs + idx;
}

static uint16_t abs_indirect(const struct CPU *cpu, uint16_t abs)
{
    if ((abs & 0x00FF) == 0x00FF)
        /* emulate page boundary hardware bug */
        return (peek_byte(cpu, abs & 0xFF00) << 8) | peek_byte(cpu, abs);
    else
        /* normal behavior */
        return peek_word(cpu, abs);
}

static uint16_t zp_indirect(const struct CPU *cpu, uint8_t zp)
{
    const uint16_t lo = peek_byte(cpu, zp & 0xFF);
    const uint16_t hi = peek_byte(cpu, (zp + 1) & 0xFF);

    return (hi << 8) | lo;
}

static uint16_t fetch_address(struct CPU *cpu, int mode, int *page_crossed)
{
    *page_crossed = 0;

    switch (mode) {

    case ABS:
        return fetch_word(cpu);

    case ABX:
        return abs_index(fetch_word(cpu), cpu->x, page_crossed);

    case ABY:
        return abs_index(fetch_word(cpu), cpu->y, page_crossed);

    case ACC:
        /* no address for register */
        return 0;

    case IMM:
        /* address where the immediate value is stored */
        return cpu->pc++;

    case IMP:
        /* no address */
        return 0;

    case IND:
        return abs_indirect(cpu, fetch_word(cpu));

    case IZX:
        {
            /* addr = {[arg + X], [arg + X + 1]} */
            return zp_indirect(cpu, fetch(cpu) + cpu->x);
        }

    case IZY:
        {
            /* addr = {[arg], [arg + 1]} + Y */
            const uint16_t addr = zp_indirect(cpu, fetch(cpu));
            if (is_page_crossing(addr, cpu->y))
                *page_crossed = 1;
            return addr + cpu->y;
        }

    case REL:
        {
            /* fetch data first, then add it to the pc */
            const uint8_t offset = fetch(cpu);
            return cpu->pc + (int8_t) offset;
        }

    case ZPG:
        return fetch(cpu);

    case ZPX:
        return (fetch(cpu) + cpu->x) & 0x00FF;

    case ZPY:
        return (fetch(cpu) + cpu->y) & 0x00FF;

    default:
        return 0;
    }
}

enum opcode {
    /* illegal */
    ILL = 0,
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
    /* XXX undocumented. there are actually some more ops. */
    LAX, SAX, DCP, ISC, SLO, RLA, SRE, RRA,
    /* no op */
    NOP
};

static const uint8_t opcode_table[] = {
/*      00   01   02   03   04   05   06   07   08   09   0A   0B   0C   0D   0E   0F */
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

#define ILLEG "???"
static const char opcode_name_table[][4] = {
"BRK","ORA",ILLEG,"SLO","NOP","ORA","ASL","SLO","PHP","ORA","ASL",ILLEG,"NOP","ORA","ASL","SLO",
"BPL","ORA",ILLEG,"SLO","NOP","ORA","ASL","SLO","CLC","ORA","NOP","SLO","NOP","ORA","ASL","SLO",
"JSR","AND",ILLEG,"RLA","BIT","AND","ROL","RLA","PLP","AND","ROL",ILLEG,"BIT","AND","ROL","RLA",
"BMI","AND",ILLEG,"RLA","NOP","AND","ROL","RLA","SEC","AND","NOP","RLA","NOP","AND","ROL","RLA",
"RTI","EOR",ILLEG,"SRE","NOP","EOR","LSR","SRE","PHA","EOR","LSR",ILLEG,"JMP","EOR","LSR","SRE",
"BVC","EOR",ILLEG,"SRE","NOP","EOR","LSR","SRE","CLI","EOR","NOP","SRE","NOP","EOR","LSR","SRE",
"RTS","ADC",ILLEG,"RRA","NOP","ADC","ROR","RRA","PLA","ADC","ROR",ILLEG,"JMP","ADC","ROR","RRA",
"BVS","ADC",ILLEG,"RRA","NOP","ADC","ROR","RRA","SEI","ADC","NOP","RRA","NOP","ADC","ROR","RRA",
"NOP","STA","NOP","SAX","STY","STA","STX","SAX","DEY","NOP","TXA",ILLEG,"STY","STA","STX","SAX",
"BCC","STA",ILLEG,ILLEG,"STY","STA","STX","SAX","TYA","STA","TXS",ILLEG,ILLEG,"STA",ILLEG,ILLEG,
"LDY","LDA","LDX","LAX","LDY","LDA","LDX","LAX","TAY","LDA","TAX","LAX","LDY","LDA","LDX","LAX",
"BCS","LDA",ILLEG,"LAX","LDY","LDA","LDX","LAX","CLV","LDA","TSX",ILLEG,"LDY","LDA","LDX","LAX",
"CPY","CMP","NOP","DCP","CPY","CMP","DEC","DCP","INY","CMP","DEX",ILLEG,"CPY","CMP","DEC","DCP",
"BNE","CMP",ILLEG,"DCP","NOP","CMP","DEC","DCP","CLD","CMP","NOP","DCP","NOP","CMP","DEC","DCP",
"CPX","SBC","NOP","ISC","CPX","SBC","INC","ISC","INX","SBC","NOP","SBC","CPX","SBC","INC","ISC",
"BEQ","SBC",ILLEG,"ISC","NOP","SBC","INC","ISC","SED","SBC","NOP","ISC","NOP","SBC","INC","ISC"
};
#undef ILLEG

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
    cpu->pc = addr;
}

static void set_flag(struct CPU *cpu, uint8_t flag, uint8_t val)
{
    if (val)
        cpu->p |= flag;
    else
        cpu->p &= ~flag;
}

static uint8_t get_flag(const struct CPU *cpu, uint8_t flag)
{
    return (cpu->p & flag) > 0;
}

static uint8_t update_zn(struct CPU *cpu, uint8_t val)
{
    set_flag(cpu, Z, val == 0x00);
    set_flag(cpu, N, val & 0x80);
    return val;
}

static void set_a(struct CPU *cpu, uint8_t val)
{
    cpu->a = val;
    update_zn(cpu, cpu->a);
}

static void set_x(struct CPU *cpu, uint8_t val)
{
    cpu->x = val;
    update_zn(cpu, cpu->x);
}

static void set_y(struct CPU *cpu, uint8_t val)
{
    cpu->y = val;
    update_zn(cpu, cpu->y);
}

static void set_s(struct CPU *cpu, uint8_t val)
{
    cpu->s = val;
}

static void set_p(struct CPU *cpu, uint8_t val)
{
    cpu->p = val | U;
}

static void push(struct CPU *cpu, uint8_t val)
{
    write_byte(cpu, 0x0100 | cpu->s, val);
    set_s(cpu, cpu->s - 1);
}

static uint8_t pop(struct CPU *cpu)
{
    set_s(cpu, cpu->s + 1);
    return read_byte(cpu, 0x0100 | cpu->s);
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
    const uint16_t a = cpu->a;
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
        write_byte(cpu, addr, cpu->a);
        break;

    /* Store Index Register X in Memory: X -> M () */
    case STX:
        write_byte(cpu, addr, cpu->x);
        break;

    /* Store Index Register Y in Memory: Y -> M () */
    case STY:
        write_byte(cpu, addr, cpu->y);
        break;

    /* Transfer Accumulator to Index X: A -> X (N, Z) */
    case TAX:
        set_x(cpu, cpu->a);
        break;

    /* Transfer Accumulator to Index Y: A -> Y (N, Z) */
    case TAY:
        set_y(cpu, cpu->a);
        break;

    /* Transfer Stack Pointer to Index X: S -> X (N, Z) */
    case TSX:
        set_x(cpu, cpu->s);
        break;

    /* Transfer Index X to Accumulator: X -> A (N, Z) */
    case TXA:
        set_a(cpu, cpu->x);
        break;

    /* Transfer Index X to Stack Pointer: X -> S () */
    case TXS:
        set_s(cpu, cpu->x);
        break;

    /* Transfer Index Y to Accumulator: Y -> A (N, Z) */
    case TYA:
        set_a(cpu, cpu->y);
        break;

    /* Push Accumulator on Stack: () */
    case PHA:
        push(cpu, cpu->a);
        break;

    /* Push Processor Status on Stack: () */
    case PHP:
        push(cpu, cpu->p | B);
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
            const uint8_t data = cpu->a;
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
            const uint8_t data = cpu->a;
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
            const uint8_t data = cpu->a;
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
            const uint8_t data = cpu->a;
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
        set_a(cpu, cpu->a & read_byte(cpu, addr));
        break;

    /* Exclusive OR Memory with Accumulator: A ^ M -> A (N, Z) */
    case EOR:
        set_a(cpu, cpu->a ^ read_byte(cpu, addr));
        break;

    /* OR Memory with Accumulator: A | M -> A (N, Z) */
    case ORA:
        set_a(cpu, cpu->a | read_byte(cpu, addr));
        break;

    /* Test Bits in Memory with Accumulator: A & M (N, V, Z) */
    case BIT:
        {
            const uint8_t data = read_byte(cpu, addr);
            set_flag(cpu, Z, (cpu->a & data) == 0x00);
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
        compare(cpu, cpu->a, read_byte(cpu, addr));
        break;

    /* Compare Index Register X to Memory: X - M (N, Z, C) */
    case CPX:
        compare(cpu, cpu->x, read_byte(cpu, addr));
        break;

    /* Compare Index Register Y to Memory: Y - M (N, Z, C) */
    case CPY:
        compare(cpu, cpu->y, read_byte(cpu, addr));
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
        set_x(cpu, cpu->x + 1);
        break;

    /* Increment Index Register Y by One: Y + 1 -> Y (N, Z) */
    case INY:
        set_y(cpu, cpu->y + 1);
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
        set_x(cpu, cpu->x - 1);
        break;

    /* Decrement Index Register Y by One: Y - 1 -> Y (N, Z) */
    case DEY:
        set_y(cpu, cpu->y - 1);
        break;

    /* Jump Indirect: [PC + 1] -> PCL, [PC + 2] -> PCH () */
    case JMP:
        set_pc(cpu, addr);
        break;

    /* Jump to Subroutine: push(PC + 2), [PC + 1] -> PCL, [PC + 2] -> PCH () */
    case JSR:
        /* the last byte of the jump instruction */
        push_word(cpu, cpu->pc - 1);
        set_pc(cpu, addr);
        break;

    /* Break Command: push(PC + 2), [FFFE] -> PCL, [FFFF] ->PCH (I) */
    case BRK:
        /* an extra byte of spacing for a break mark */
        push_word(cpu, cpu->pc + 1);
        push(cpu, cpu->p | B);

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
        set_pc(cpu, cpu->pc + 1);
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

    /* XXX Undocumented -------------------------------- */

    /* Load Accumulator and Index Register X from Memory: M -> A, X (N, Z) */
    case LAX:
        set_a(cpu, read_byte(cpu, addr));
        set_x(cpu, cpu->a);
        break;

    /* Store Accumulator AND Index Register X in Memory: A & X -> M () */
    case SAX:
        write_byte(cpu, addr, cpu->a & cpu->x);
        break;

    /* Decrement Memory by One then Compare with Accumulator: M - 1 -> M, A - M (N, Z, C) */
    case DCP:
        {
            const uint8_t data = read_byte(cpu, addr) - 1;
            update_zn(cpu, data);
            write_byte(cpu, addr, data);
            compare(cpu, cpu->a, data);
        }
        break;

    /* Increment Memory by One then SBC then Subtract Memory from Accumulator with Borrow:
     * M + 1 -> M, A - M -> A (N, V, Z, C) */
    case ISC:
        {
            const uint8_t data = read_byte(cpu, addr) + 1;
            update_zn(cpu, data);
            write_byte(cpu, addr, data);
            add_a_m(cpu, ~data);
        }
        break;

    /* Arithmetic Shift Left then OR Memory with Accumulator:
     * M * 2 -> M, A | M -> A (N, Z, C) */
    case SLO:
        {
            uint8_t data = read_byte(cpu, addr);
            set_flag(cpu, C, data & 0x80);
            data <<= 1;
            update_zn(cpu, data);
            write_byte(cpu, addr, data);
            set_a(cpu, cpu->a | data);
        }
        break;

    /* Rotate Left then AND with Accumulator: C <- /M7...M0/ <- C, A & M -> A (N, Z, C) */
    case RLA:
        {
            const uint8_t carry = get_flag(cpu, C);
            uint8_t data = read_byte(cpu, addr);
            set_flag(cpu, C, data & 0x80);
            data = (data << 1) | carry;
            update_zn(cpu, data);
            write_byte(cpu, addr, data);
            set_a(cpu, cpu->a & data);
        }
        break;

    /* Logical Shift Right then Exclusive OR Memory with Accumulator:
     * M /2 -> M, M ^ A -> A (N, Z, C) */
    case SRE:
        {
            uint8_t data = read_byte(cpu, addr);
            set_flag(cpu, C, data & 0x01);
            data >>= 1;
            update_zn(cpu, data);
            write_byte(cpu, addr, data);
            set_a(cpu, cpu->a ^ data);
        }
        break;

    /* Rotate Right and Add Memory to Accumulator:
     * C -> /M7...M0/ -> C, A + M + C -> A (N, V, Z, C) */
    case RRA:
        {
            const uint8_t carry = get_flag(cpu, C);
            uint8_t data = read_byte(cpu, addr);
            set_flag(cpu, C, data & 0x01);
            data = (data >> 1) | (carry << 7);
            update_zn(cpu, data);
            write_byte(cpu, addr, data);
            add_a_m(cpu, data);
        }
        break;

    default:
        break;
    }

    return inst.cycles + page_crossed + branch_taken;
}

void power_up_cpu(struct CPU *cpu)
{
    set_pc(cpu, read_word(cpu, 0xFFFC));

    set_a(cpu, 0x00);
    set_x(cpu, 0x00);
    set_y(cpu, 0x00);
    set_s(cpu, 0xFD);
    set_p(cpu, 0x00 | I);

    power_up_apu(&cpu->apu);
}

void reset_cpu(struct CPU *cpu)
{
    set_pc(cpu, read_word(cpu, 0xFFFC));

    set_s(cpu, cpu->s - 3);
    set_flag(cpu, I, 1);

    /* takes cycles */
    cpu->cycles = 8;

    reset_apu(&cpu->apu);
}

void nmi(struct CPU *cpu)
{
    push_word(cpu, cpu->pc);

    set_flag(cpu, B, 0);
    set_flag(cpu, I, 1);
    push(cpu, cpu->p);

    set_pc(cpu, read_word(cpu, 0xFFFA));
    cpu->cycles = 8;
}

void clock_cpu(struct CPU *cpu)
{
    if (cpu->cycles == 0) {
        uint8_t code, cycs;
        struct instruction inst;

        code = fetch(cpu);
        inst = decode(code);
        cycs = execute(cpu, inst);

        cpu->cycles = cycs;
    }

    cpu->cycles--;
}

void set_controller_input(struct CPU *cpu, uint8_t id, uint8_t input)
{
    cpu->controller_input[id] = input;
}

int is_suspended(const struct CPU *cpu)
{
    return cpu->suspended;
}

void resume(struct CPU *cpu)
{
    cpu->suspended = 0;
    cpu->dma_page = 0x00;
}

uint8_t get_dma_page(const struct CPU *cpu)
{
    return cpu->dma_page;
}

uint8_t read_cpu_data(const struct CPU *cpu, uint16_t addr)
{
    return peek_byte(cpu, addr);
}

void get_cpu_status(const struct CPU *cpu, struct cpu_status *stat)
{
    stat->pc = cpu->pc;
    stat->a  = cpu->a;
    stat->x  = cpu->x;
    stat->y  = cpu->y;
    stat->p  = cpu->p;
    stat->s  = cpu->s;

    stat->code = peek_byte(cpu, stat->pc + 0);
    stat->lo   = peek_byte(cpu, stat->pc + 1);
    stat->hi   = peek_byte(cpu, stat->pc + 2);
    stat->wd   = (stat->hi << 8) | stat->lo;

    strcpy(stat->inst_name, opcode_name_table[stat->code]);
    strcpy(stat->mode_name, addr_mode_name_table[addr_mode_table[stat->code]]);
}

uint8_t peek_cpu_data(const struct CPU *cpu, uint16_t addr)
{
    return peek_byte(cpu, addr);
}
