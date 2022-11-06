#include <cstring>
#include "cpu.h"
#include "ppu.h"
#include "cartridge.h"

namespace nes {

enum status_flag {
    C = 1 << 0, // carry
    Z = 1 << 1, // zero
    I = 1 << 2, // disable interrupts
    D = 1 << 3, // decimal mode
    B = 1 << 4, // break
    U = 1 << 5, // unused
    V = 1 << 6, // overflow
    N = 1 << 7  // negative
};

void CPU::write_byte(uint16_t addr, uint8_t data)
{
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        wram_[addr & 0x07FF] = data;
    }
    else if (addr == 0x2000) {
        ppu_.WriteControl(data);
    }
    else if (addr == 0x2001) {
        ppu_.WriteMask(data);
    }
    else if (addr == 0x2002) {
        // PPU status not writable
    }
    else if (addr == 0x2003) {
        ppu_.WriteOamAddress(data);
    }
    else if (addr == 0x2004) {
        ppu_.WriteOamData(data);
    }
    else if (addr == 0x2005) {
        ppu_.WriteScroll(data);
    }
    else if (addr == 0x2006) {
        ppu_.WriteAddress(data);
    }
    else if (addr == 0x2007) {
        ppu_.WriteData(data);
    }
    else if (addr >= 0x2008 && addr <= 0x3FFF) {
        // PPU register mirrored every 8
        write_byte(0x2000 | (addr & 0x007), data);
    }
    else if (addr == 0x4000) {
        apu_.WriteSquare1Volume(data);
    }
    else if (addr == 0x4001) {
        apu_.WriteSquare1Sweep(data);
    }
    else if (addr == 0x4002) {
        apu_.WriteSquare1Lo(data);
    }
    else if (addr == 0x4003) {
        apu_.WriteSquare1Hi(data);
    }
    else if (addr == 0x4004) {
        apu_.WriteSquare2Volume(data);
    }
    else if (addr == 0x4005) {
        apu_.WriteSquare2Sweep(data);
    }
    else if (addr == 0x4006) {
        apu_.WriteSquare2Lo(data);
    }
    else if (addr == 0x4007) {
        apu_.WriteSquare2Hi(data);
    }
    else if (addr == 0x4008) {
        apu_.WriteTriangleLinear(data);
    }
    else if (addr == 0x400A) {
        apu_.WriteTriangleLo(data);
    }
    else if (addr == 0x400B) {
        apu_.WriteTriangleHi(data);
    }
    else if (addr == 0x400C) {
        apu_.WriteNoiseVolume(data);
    }
    else if (addr == 0x400E) {
        apu_.WriteNoiseLo(data);
    }
    else if (addr == 0x400F) {
        apu_.WriteNoiseHi(data);
    }
    else if (addr == 0x4014) {
        // DMA
        suspended_ = true;
        dma_page_ = data;
    }
    else if (addr == 0x4015) {
        apu_.WriteStatus(data);
    }
    else if (addr == 0x4016) {
        const int id = addr & 0x001;
        controller_state_[id] = controller_input_[id];
    }
    else if (addr == 0x4017) {
        apu_.WriteFrameCounter(data);
    }
    else {
        cart_->WriteProg(addr, data);
    }
}

uint8_t CPU::read_byte(uint16_t addr)
{
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        return wram_[addr & 0x07FF];
    }
    else if (addr == 0x2000) {
        // PPU control not readable
    }
    else if (addr == 0x2001) {
        // PPU mask not readable
    }
    else if (addr == 0x2002) {
        return ppu_.ReadStatus();
    }
    else if (addr == 0x2003) {
        // PPU oam address not readable
    }
    else if (addr == 0x2004) {
        return ppu_.ReadOamData();
    }
    else if (addr == 0x2005) {
        // PPU scroll not readable
    }
    else if (addr == 0x2006) {
        // PPU address not readable
    }
    else if (addr == 0x2007) {
        return ppu_.ReadData();
    }
    else if (addr >= 0x2008 && addr <= 0x3FFF) {
        // PPU register mirrored every 8
        return read_byte(0x2000 | (addr & 0x007));
    }
    else if (addr == 0x4015) {
        return apu_.ReadStatus();
    }
    else if (addr >= 0x4016 && addr <= 0x4017) {
        const int id = addr & 0x001;
        const uint8_t data = (controller_state_[id] & 0x80) > 0;
        controller_state_[id] <<= 1;
        return data;
    }
    else {
        return cart_->ReadProg(addr);
    }

    return 0;
}

uint8_t CPU::peek_byte(uint16_t addr) const
{
    if (addr == 0x2002) {
        return ppu_.PeekStatus();
    }
    else if (addr >= 0x4016 && addr <= 0x4017) {
        const int id = addr & 0x001;
        return (controller_state_[id] & 0x80) > 0;
    }

    return const_cast<CPU*>(this)->read_byte(addr);
}

uint16_t CPU::peek_word(uint16_t addr) const
{
    const uint16_t lo = peek_byte(addr);
    const uint16_t hi = peek_byte(addr + 1);

    return (hi << 8) | lo;
}

uint16_t CPU::read_word(uint16_t addr)
{
    const uint16_t lo = read_byte(addr);
    const uint16_t hi = read_byte(addr + 1);

    return (hi << 8) | lo;
}

uint8_t CPU::fetch()
{
    return read_byte(pc_++);
}

uint16_t CPU::fetch_word()
{
    const uint16_t lo = fetch();
    const uint16_t hi = fetch();

    return (hi << 8) | lo;
}

enum addr_mode {ABS, ABX, ABY, ACC, IMM, IMP, IND, IZX, IZY, REL, ZPG, ZPX, ZPY};

static const char addr_mode_name_table[][4] = {
    "ABS","ABX","ABY","ACC","IMM","IMP","IND","IZX","IZY","REL","ZPG","ZPX","ZPY"
};

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

static uint8_t is_page_crossing(uint16_t addr, uint8_t addend)
{
    return (addr & 0x00FF) + (addend & 0x00FF) > 0x00FF;
}

static uint16_t abs_index(uint16_t abs, uint8_t idx, int *page_crossed)
{
    *page_crossed = is_page_crossing(abs, idx);
    return abs + idx;
}

uint16_t CPU::abs_indirect(uint16_t abs) const
{
    if ((abs & 0x00FF) == 0x00FF)
        // emulate page boundary hardware bug
        return (peek_byte(abs & 0xFF00) << 8) | peek_byte(abs);
    else
        // normal behavior
        return peek_word(abs);
}

uint16_t CPU::zp_indirect(uint8_t zp) const
{
    const uint16_t lo = peek_byte(zp & 0xFF);
    const uint16_t hi = peek_byte((zp + 1) & 0xFF);

    return (hi << 8) | lo;
}

uint16_t CPU::fetch_address(int mode, int *page_crossed)
{
    *page_crossed = 0;

    switch (mode) {

    case ABS:
        return fetch_word();

    case ABX:
        return abs_index(fetch_word(), x_, page_crossed);

    case ABY:
        return abs_index(fetch_word(), y_, page_crossed);

    case ACC:
        // no address for register
        return 0;

    case IMM:
        // address where the immediate value is stored
        return pc_++;

    case IMP:
        // no address
        return 0;

    case IND:
        return abs_indirect(fetch_word());

    case IZX:
        // addr = {[arg + X], [arg + X + 1]}
        return zp_indirect(fetch() + x_);

    case IZY:
        {
            // addr = {[arg], [arg + 1]} + Y
            const uint16_t addr = zp_indirect(fetch());
            if (is_page_crossing(addr, y_))
                *page_crossed = 1;
            return addr + y_;
        }

    case REL:
        {
            // fetch data first, then add it to the PC
            const uint8_t offset = fetch();
            return pc_ + (int8_t) offset;
        }

    case ZPG:
        return fetch();

    case ZPX:
        return (fetch() + x_) & 0x00FF;

    case ZPY:
        return (fetch() + y_) & 0x00FF;

    default:
        return 0;
    }
}

enum opcode {
    // illegal
    ILL = 0,
    // load and store
    LDA, LDX, LDY, STA, STX, STY,
    // transfer
    TAX, TAY, TSX, TXA, TXS, TYA,
    // stack
    PHA, PHP, PLA, PLP,
    // shift
    ASL, LSR, ROL, ROR,
    // logic
    AND, EOR, ORA, BIT,
    // arithmetic
    ADC, SBC, CMP, CPX, CPY,
    // increment and decrement
    INC, INX, INY, DEC, DEX, DEY,
    // control
    JMP, JSR, BRK, RTI, RTS,
    // branch
    BCC, BCS, BEQ, BMI, BNE, BPL, BVC, BVS,
    // flag
    CLC, CLD, CLI, CLV, SEC, SED, SEI,
    // XXX undocumented. there are actually some more ops.
    LAX, SAX, DCP, ISC, SLO, RLA, SRE, RRA,
    // no op
    NOP
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

void CPU::set_pc(uint16_t addr)
{
    pc_ = addr;
}

void CPU::set_flag(uint8_t flag, uint8_t val)
{
    if (val)
        p_ |= flag;
    else
        p_ &= ~flag;
}

uint8_t CPU::get_flag(uint8_t flag) const
{
    return (p_ & flag) > 0;
}

uint8_t CPU::update_zn(uint8_t val)
{
    set_flag(Z, val == 0x00);
    set_flag(N, val & 0x80);
    return val;
}

void CPU::set_a(uint8_t val)
{
    a_ = val;
    update_zn(a_);
}

void CPU::set_x(uint8_t val)
{
    x_ = val;
    update_zn(x_);
}

void CPU::set_y(uint8_t val)
{
    y_ = val;
    update_zn(y_);
}

void CPU::set_s(uint8_t val)
{
    s_ = val;
}

void CPU::set_p(uint8_t val)
{
    p_ = val | U;
}

void CPU::push(uint8_t val)
{
    write_byte(0x0100 | s_, val);
    set_s(s_ - 1);
}

uint8_t CPU::pop()
{
    set_s(s_ + 1);
    return read_byte(0x0100 | s_);
}

void CPU::push_word(uint16_t val)
{
    push(val >> 8);
    push(val);
}

uint16_t CPU::pop_word()
{
    const uint16_t lo = pop();
    const uint16_t hi = pop();

    return (hi << 8) | lo;
}

void CPU::compare(uint8_t a_, uint8_t b)
{
    set_flag(C, a_ >= b);
    update_zn(a_ - b);
}

bool CPU::branch_if(uint16_t addr, bool cond)
{
    if (!cond)
        return false;

    set_pc(addr);
    return true;
}

static bool is_positive(uint8_t val)
{
    return !(val & 0x80);
}

void CPU::add_a_m(uint8_t data)
{
    const uint16_t m = data;
    const uint16_t c = get_flag(C);
    const uint16_t r = a_ + m + c;
    const int A = is_positive(a_);
    const int M = is_positive(m);
    const int R = is_positive(r);

    set_flag(C, r > 0xFF);
    set_flag(V, (A && M && !R) || (!A && !M && R));
    set_a(r);
}

static Instruction decode(uint8_t code)
{
    Instruction inst;

    inst.opcode    = opcode_table[code];
    inst.addr_mode = addr_mode_table[code];
    inst.cycles    = cycle_table[code];

    return inst;
}

int CPU::execute(Instruction inst)
{
    int page_crossed = 0;
    int branch_taken = 0;
    const uint8_t mode = inst.addr_mode;
    const uint16_t addr = fetch_address(mode, &page_crossed);

    switch (inst.opcode) {

    // Load Accumulator with Memory: M -> A (N, Z)
    case LDA:
        set_a(read_byte(addr));
        break;

    // Load Index Register X from Memory: M -> X (N, Z)
    case LDX:
        set_x(read_byte(addr));
        break;

    // Load Index Register Y from Memory: M -> Y (N, Z)
    case LDY:
        set_y(read_byte(addr));
        break;

    // Store Accumulator in Memory: A -> M ()
    case STA:
        write_byte(addr, a_);
        break;

    // Store Index Register X in Memory: X -> M ()
    case STX:
        write_byte(addr, x_);
        break;

    // Store Index Register Y in Memory: Y -> M ()
    case STY:
        write_byte(addr, y_);
        break;

    // Transfer Accumulator to Index X: A -> X (N, Z)
    case TAX:
        set_x(a_);
        break;

    // Transfer Accumulator to Index Y: A -> Y (N, Z)
    case TAY:
        set_y(a_);
        break;

    // Transfer Stack Pointer to Index X: S -> X (N, Z)
    case TSX:
        set_x(s_);
        break;

    // Transfer Index X to Accumulator: X -> A (N, Z)
    case TXA:
        set_a(x_);
        break;

    // Transfer Index X to Stack Pointer: X -> S ()
    case TXS:
        set_s(x_);
        break;

    // Transfer Index Y to Accumulator: Y -> A (N, Z)
    case TYA:
        set_a(y_);
        break;

    // Push Accumulator on Stack: ()
    case PHA:
        push(a_);
        break;

    // Push Processor Status on Stack: ()
    case PHP:
        push(p_ | B);
        break;

    // Pull Accumulator from Stack: (N, Z)
    case PLA:
        set_a(pop());
        break;

    // Pull Processor Status from Stack: (N, V, D, I, Z, C)
    case PLP:
        set_p(pop());
        set_flag(B, 0);
        break;

    // Arithmetic Shift Left: C <- /M7...M0/ <- 0 (N, Z, C)
    case ASL:
        if (mode == ACC) {
            const uint8_t data = a_;
            set_flag(C, data & 0x80);
            set_a(data << 1);
        } else {
            uint8_t data = read_byte(addr);
            set_flag(C, data & 0x80);
            data <<= 1;
            update_zn(data);
            write_byte(addr, data);
        }
        break;

    // Logical Shift Right: 0 -> /M7...M0/ -> C (N, Z, C)
    case LSR:
        if (mode == ACC) {
            const uint8_t data = a_;
            set_flag(C, data & 0x01);
            set_a(data >> 1);
        } else {
            uint8_t data = read_byte(addr);
            set_flag(C, data & 0x01);
            data >>= 1;
            update_zn(data);
            write_byte(addr, data);
        }
        break;

    // Rotate Left: C <- /M7...M0/ <- C (N, Z, C)
    case ROL:
        if (mode == ACC) {
            const uint8_t carry = get_flag(C);
            const uint8_t data = a_;
            set_flag(C, data & 0x80);
            set_a((data << 1) | carry);
        } else {
            const uint8_t carry = get_flag(C);
            uint8_t data = read_byte(addr);
            set_flag(C, data & 0x80);
            data = (data << 1) | carry;
            update_zn(data);
            write_byte(addr, data);
        }
        break;

    // Rotate Right: C -> /M7...M0/ -> C (N, Z, C)
    case ROR:
        if (mode == ACC) {
            const uint8_t carry = get_flag(C);
            const uint8_t data = a_;
            set_flag(C, data & 0x01);
            set_a((data >> 1) | (carry << 7));
        } else {
            const uint8_t carry = get_flag(C);
            uint8_t data = read_byte(addr);
            set_flag(C, data & 0x01);
            data = (data >> 1) | (carry << 7);
            update_zn(data);
            write_byte(addr, data);
        }
        break;

    // AND Memory with Accumulator: A & M -> A (N, Z)
    case AND:
        set_a(a_ & read_byte(addr));
        break;

    // Exclusive OR Memory with Accumulator: A ^ M -> A (N, Z)
    case EOR:
        set_a(a_ ^ read_byte(addr));
        break;

    // OR Memory with Accumulator: A | M -> A (N, Z)
    case ORA:
        set_a(a_ | read_byte(addr));
        break;

    // Test Bits in Memory with Accumulator: A & M (N, V, Z)
    case BIT:
        {
            const uint8_t data = read_byte(addr);
            set_flag(Z, (a_ & data) == 0x00);
            set_flag(N, data & N);
            set_flag(V, data & V);
        }
        break;

    // Add Memory to Accumulator with Carry: A + M + C -> A, C (N, V, Z, C)
    case ADC:
        add_a_m(read_byte(addr));
        break;

    // Subtract Memory to Accumulator with Carry: A - M - ~C -> A (N, V, Z, C)
    case SBC:
        // A - M - ~C = A + (-M) - (1 - C)
        //            = A + (-M) - 1 + C
        //            = A + (~M + 1) - 1 + C
        //            = A + (~M) + C
        add_a_m(~read_byte(addr));
        break;

    // Compare Memory and Accumulator: A - M (N, Z, C)
    case CMP:
        compare(a_, read_byte(addr));
        break;

    // Compare Index Register X to Memory: X - M (N, Z, C)
    case CPX:
        compare(x_, read_byte(addr));
        break;

    // Compare Index Register Y to Memory: Y - M (N, Z, C)
    case CPY:
        compare(y_, read_byte(addr));
        break;

    // Increment Memory by One: M + 1 -> M (N, Z)
    case INC:
        {
            const uint8_t data = read_byte(addr) + 1;
            update_zn(data);
            write_byte(addr, data);
        }
        break;

    // Increment Index Register X by One: X + 1 -> X (N, Z)
    case INX:
        set_x(x_ + 1);
        break;

    // Increment Index Register Y by One: Y + 1 -> Y (N, Z)
    case INY:
        set_y(y_ + 1);
        break;

    // Decrement Memory by One: M - 1 -> M (N, Z)
    case DEC:
        {
            const uint8_t data = read_byte(addr) - 1;
            update_zn(data);
            write_byte(addr, data);
        }
        break;

    // Decrement Index Register X by One: X - 1 -> X (N, Z)
    case DEX:
        set_x(x_ - 1);
        break;

    // Decrement Index Register Y by One: Y - 1 -> Y (N, Z)
    case DEY:
        set_y(y_ - 1);
        break;

    // Jump Indirect: [PC + 1] -> PCL, [PC + 2] -> PCH ()
    case JMP:
        set_pc(addr);
        break;

    // Jump to Subroutine: push(PC + 2), [PC + 1] -> PCL, [PC + 2] -> PCH ()
    case JSR:
        // the last byte of the jump instruction
        push_word(pc_ - 1);
        set_pc(addr);
        break;

    // Break Command: push(PC + 2), [FFFE] -> PCL, [FFFF] ->PCH (I)
    case BRK:
        // an extra byte of spacing for a break mark
        push_word(pc_ + 1);
        push(p_ | B);

        set_flag(I, 1);
        set_pc(read_word(0xFFFE));
        break;

    // Return from Interrupt: pop(P) pop(PC) (N, V, D, I, Z, C)
    case RTI:
        set_p(pop());
        set_flag(B, 0);
        set_pc(pop_word());
        break;

    // Return from Subroutine: pop(PC), PC + 1 -> PC ()
    case RTS:
        set_pc(pop_word());
        set_pc(pc_ + 1);
        break;

    // Branch on Carry Clear: ()
    case BCC:
        branch_taken = branch_if(addr, get_flag(C) == 0);
        break;

    // Branch on Carry Set: ()
    case BCS:
        branch_taken = branch_if(addr, get_flag(C) == 1);
        break;

    // Branch on Result Zero: ()
    case BEQ:
        branch_taken = branch_if(addr, get_flag(Z) == 1);
        break;

    // Branch on Result Minus: ()
    case BMI:
        branch_taken = branch_if(addr, get_flag(N) == 1);
        break;

    // Branch on Result Not Zero: ()
    case BNE:
        branch_taken = branch_if(addr, get_flag(Z) == 0);
        break;

    // Branch on Result Plus: ()
    case BPL:
        branch_taken = branch_if(addr, get_flag(N) == 0);
        break;

    // Branch on Overflow Clear: ()
    case BVC:
        branch_taken = branch_if(addr, get_flag(V) == 0);
        break;

    // Branch on Overflow Set: ()
    case BVS:
        branch_taken = branch_if(addr, get_flag(V) == 1);
        break;

    // Clear Carry Flag: 0 -> C (C)
    case CLC:
        set_flag(C, 0);
        break;

    // Clear Decimal Mode: 0 -> D (D)
    case CLD:
        set_flag(D, 0);
        break;

    // Clear Interrupt Disable: 0 -> I (I)
    case CLI:
        set_flag(I, 0);
        break;

    // Clear Overflow Flag: 0 -> V (V)
    case CLV:
        set_flag(V, 0);
        break;

    // Set Carry Flag: 1 -> C (C)
    case SEC:
        set_flag(C, 1);
        break;

    // Set Decimal Mode: 1 -> D (D)
    case SED:
        set_flag(D, 1);
        break;

    // Set Interrupt Disable: 1 -> I (I)
    case SEI:
        set_flag(I, 1);
        break;

    // No Operation: ()
    case NOP:
        break;

    // XXX Undocumented --------------------------------

    // Load Accumulator and Index Register X from Memory: M -> A, X (N, Z)
    case LAX:
        set_a(read_byte(addr));
        set_x(a_);
        break;

    // Store Accumulator AND Index Register X in Memory: A & X -> M ()
    case SAX:
        write_byte(addr, a_ & x_);
        break;

    // Decrement Memory by One then Compare with Accumulator: M - 1 -> M, A - M (N, Z, C)
    case DCP:
        {
            const uint8_t data = read_byte(addr) - 1;
            update_zn(data);
            write_byte(addr, data);
            compare(a_, data);
        }
        break;

    // Increment Memory by One then SBC then Subtract Memory from Accumulator with Borrow:
    // M + 1 -> M, A - M -> A (N, V, Z, C)
    case ISC:
        {
            const uint8_t data = read_byte(addr) + 1;
            update_zn(data);
            write_byte(addr, data);
            add_a_m(~data);
        }
        break;

    // Arithmetic Shift Left then OR Memory with Accumulator:
    // M * 2 -> M, A | M -> A (N, Z, C)
    case SLO:
        {
            uint8_t data = read_byte(addr);
            set_flag(C, data & 0x80);
            data <<= 1;
            update_zn(data);
            write_byte(addr, data);
            set_a(a_ | data);
        }
        break;

    // Rotate Left then AND with Accumulator: C <- /M7...M0/ <- C, A & M -> A (N, Z, C)
    case RLA:
        {
            const uint8_t carry = get_flag(C);
            uint8_t data = read_byte(addr);
            set_flag(C, data & 0x80);
            data = (data << 1) | carry;
            update_zn(data);
            write_byte(addr, data);
            set_a(a_ & data);
        }
        break;

    // Logical Shift Right then Exclusive OR Memory with Accumulator:
    // M /2 -> M, M ^ A -> A (N, Z, C)
    case SRE:
        {
            uint8_t data = read_byte(addr);
            set_flag(C, data & 0x01);
            data >>= 1;
            update_zn(data);
            write_byte(addr, data);
            set_a(a_ ^ data);
        }
        break;

    // Rotate Right and Add Memory to Accumulator:
    // C -> /M7...M0/ -> C, A + M + C -> A (N, V, Z, C)
    case RRA:
        {
            const uint8_t carry = get_flag(C);
            uint8_t data = read_byte(addr);
            set_flag(C, data & 0x01);
            data = (data >> 1) | (carry << 7);
            update_zn(data);
            write_byte(addr, data);
            add_a_m(data);
        }
        break;

    default:
        break;
    }

    return inst.cycles + page_crossed + branch_taken;
}

void CPU::SetCartride(Cartridge *cart)
{
    cart_ = cart;
}

void CPU::PowerUp()
{
    set_pc(read_word(0xFFFC));

    set_a(0x00);
    set_x(0x00);
    set_y(0x00);
    set_s(0xFD);
    set_p(0x00 | I);

    apu_.PowerUp();
}

void CPU::Reset()
{
    set_pc(read_word(0xFFFC));

    set_s(s_ - 3);
    set_flag(I, 1);

    // takes cycles
    cycles_ = 8;

    apu_.Reset();
}

void CPU::Clock()
{
    if (cycles_ == 0) {
        uint8_t code, cycs;
        Instruction inst;

        code = fetch();
        inst = decode(code);
        cycs = execute(inst);

        cycles_ = cycs;
    }

    cycles_--;
}

void CPU::ClockAPU()
{
    apu_.Clock();
}

void CPU::HandleIRQ()
{
    if (get_flag(I))
        // interrupt is not allowed
        return;

    push_word(pc_);

    set_flag(B, 0);
    set_flag(I, 1);
    push(p_);

    set_pc(read_word(0xFFFE));
    cycles_ = 7;
}

void CPU::HandleNMI()
{
    push_word(pc_);

    set_flag(B, 0);
    set_flag(I, 1);
    push(p_);

    set_pc(read_word(0xFFFA));
    cycles_ = 8;
}

void CPU::ClearIRQ()
{
    apu_.ClearIRQ();
}

bool CPU::IsSetIRQ() const
{
    return apu_.IsSetIRQ();
}

void CPU::InputController(uint8_t controller_id, uint8_t input)
{
    controller_input_[controller_id] = input;
}

bool CPU::IsSuspended() const
{
    return suspended_;
}

void CPU::Resume()
{
    suspended_ = false;
    dma_page_ = 0x00;
}

uint8_t CPU::GetDmaPage() const
{
    return dma_page_;
}

uint8_t CPU::PeekData(uint16_t addr) const
{
    return peek_byte(addr);
}

void CPU::GetStatus(CpuStatus &stat) const
{
    stat.pc = pc_;
    stat.a  = a_;
    stat.x  = x_;
    stat.y  = y_;
    stat.p  = p_;
    stat.s  = s_;

    stat.code = peek_byte(stat.pc + 0);
    stat.lo   = peek_byte(stat.pc + 1);
    stat.hi   = peek_byte(stat.pc + 2);
    stat.wd   = (stat.hi << 8) | stat.lo;

    strcpy(stat.inst_name, opcode_name_table[stat.code]);
    strcpy(stat.mode_name, addr_mode_name_table[addr_mode_table[stat.code]]);
}

void CPU::SetPC(uint16_t addr)
{
    pc_ = addr;
}

int CPU::GetCycles() const
{
    return cycles_;
}

} // namespace
