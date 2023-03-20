#include "disassemble.h"
#include <cassert>
#include <cstdio>

namespace nes {

Assembly::Assembly()
{
}

Assembly::~Assembly()
{
}

int Assembly::FindCode(uint16_t addr) const
{
    auto found = addr_to_code_.find(addr);

    if (found != addr_to_code_.end())
        return found->second;
    else
        return -1;
}

Code Assembly::GetCode(int index) const
{
    assert(index >= 0 && index < GetCount());

    return codes_[index];
}

int Assembly::GetCount() const
{
    return codes_.size();
}

Code DisassembleLine(const CPU &cpu, uint16_t addr)
{
    const uint8_t opcode = cpu.PeekData(addr);
    const Instruction inst = Decode(opcode);

    Code code;
    code.instruction = inst;
    code.address = addr;
    code.opcode  = opcode;
    code.lo      = cpu.PeekData(addr + 1);
    code.hi      = cpu.PeekData(addr + 2);
    code.word    = (code.hi << 8) | code.lo;

    return code;
}

void Assembly::DisassembleProgram(const CPU &cpu)
{
    uint32_t addr = 0x0000;

    while (addr <= 0xFFFF) {
        const Code code = DisassembleLine(cpu, addr);
        const uint32_t index_to_add = codes_.size();

        codes_.push_back(code);
        addr_to_code_.insert({addr, index_to_add});

        addr += code.instruction.bytes;
    }
}

std::string GetCodeString(const Code &code)
{
    constexpr size_t SIZE = 64;
    char buf[SIZE] = {'\0'};
    const char *op_name = GetOperationName(code.instruction.operation);
    const uint16_t addr = code.address;
    const uint8_t op = code.opcode;
    const uint8_t lo = code.lo;
    const uint8_t hi = code.hi;
    const uint16_t wd = code.word;

    switch (code.instruction.bytes) {
    case 1:
        snprintf(buf, SIZE, "%04X  %02X        %s", addr, op, op_name);
        break;

    case 2:
        snprintf(buf, SIZE, "%04X  %02X %02X     %s", addr, op, lo, op_name);
        break;

    case 3:
        snprintf(buf, SIZE, "%04X  %02X %02X %02X  %s", addr, op, lo, hi, op_name);
        break;
    }

    // Unofficial instruction marker
    switch (code.instruction.operation) {
    case LAX: case SAX: case DCP: case ISC: case SLO: case RLA: case SRE: case RRA:
        buf[15] = '*';
        break;

    case SBC:
        buf[15] = code.opcode == 0xEB ? '*' : ' ';
        break;

    case NOP:
        buf[15] = code.opcode != 0xEA ? '*' : ' ';
        break;
    }

    std::string result = buf;

    switch (code.instruction.addr_mode) {
    case IND:
        snprintf(buf, SIZE, "($%04X)", wd);
        break;

    case ABS:
        snprintf(buf, SIZE, "$%04X", wd);
        break;

    case ABX:
        snprintf(buf, SIZE, "$%04X,X", wd);
        break;

    case ABY:
        snprintf(buf, SIZE, "$%04X,Y", wd);
        break;

    case IZX:
        snprintf(buf, SIZE, "($%02X,X)", lo);
        break;

    case IZY:
        snprintf(buf, SIZE, "($%02X),Y", lo);
        break;

    case REL:
        snprintf(buf, SIZE, "$%04X", (addr + 2) + static_cast<int8_t>(lo));
        break;

    case ZPG:
        snprintf(buf, SIZE, "$%02X", lo);
        break;

    case ZPX:
        snprintf(buf, SIZE, "$%02X,X", lo);
        break;

    case ZPY:
        snprintf(buf, SIZE, "$%02X,Y", lo);
        break;

    case IMM:
        snprintf(buf, SIZE, "#$%02X", lo);
        break;

    case ACC:
        snprintf(buf, SIZE, "A");
        break;

    case IMP:
    default:
        buf[0] = '\0';
        break;
    }

    if (buf[0])
        result += std::string(" ") + buf;

    return result;
}

std::string GetMemoryString(const Code &code, const CPU &cpu)
{
    constexpr size_t SIZE = 32;
    char buf[SIZE] = {'\0'};
    const uint8_t lo = code.lo;
    const uint16_t wd = code.word;

    const CpuStatus stat = cpu.GetStatus();
    const uint8_t  x = stat.x;
    const uint8_t  y = stat.y;

    switch (code.instruction.addr_mode) {
    case IND:
        snprintf(buf, SIZE, " = %04X", cpu.GetAbsoluteIndirect(wd));
        break;

    case ABS:
        if (code.instruction.operation != JMP && code.instruction.operation != JSR)
            snprintf(buf, SIZE, " = %02X", cpu.PeekData(wd));
        break;

    case ABX:
        snprintf(buf, SIZE, " @ %04X = %02X", wd + x, cpu.PeekData(wd + x));
        break;

    case ABY:
        snprintf(buf, SIZE, " @ %04X = %02X",
                (wd + y) & 0xFFFF, cpu.PeekData((wd + y) & 0xFFFF));
        break;

    case IZX:
        {
            const uint16_t zpi = cpu.GetZeroPageIndirect(lo + x);
            snprintf(buf, SIZE, " @ %02X = %04X = %02X",
                    (lo + x) & 0xFF, zpi, cpu.PeekData(zpi));
        }
        break;

    case IZY:
        {
            const uint16_t zpi = cpu.GetZeroPageIndirect(lo);
            snprintf(buf, SIZE, " = %04X @ %04X = %02X",
                    zpi, (zpi + y) & 0xFFFF, cpu.PeekData(zpi + y));
        }
        break;

    case ZPX:
        snprintf(buf, SIZE, " @ %02X = %02X", (lo + x) & 0xFF, cpu.PeekData((lo + x) & 0xFF));
        break;

    case ZPY:
        snprintf(buf, SIZE, " @ %02X = %02X", (lo + y) & 0xFF, cpu.PeekData((lo + y) & 0xFF));
        break;

    case ZPG:
        snprintf(buf, SIZE, " = %02X", cpu.PeekData(lo));
        break;

    default:
        break;
    }

    return buf;
}

} // namespace
