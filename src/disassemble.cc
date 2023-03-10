#include <cstdio>
#include "disassemble.h"

namespace nes {

void Disassemble(AssemblyCode &assem, const Cartridge &cart)
{
    uint32_t addr = 0x0000;

    while (addr <= 0xFFFF) {
        Code line;
        const uint8_t code = cart.ReadProg(addr);
        const Instruction inst = Decode(code);

        line.inst = inst;
        line.addr = addr;
        line.code = code;
        line.lo   = cart.ReadProg(addr + 1);
        line.hi   = cart.ReadProg(addr + 2);
        line.wd   = (line.hi << 8) | line.lo;

        const uint32_t index_to_add = assem.instructions_.size();
        assem.instructions_.push_back(line);
        assem.addr_map_.insert({addr, index_to_add});

        addr += inst.bytes;
    }
}

std::string GetCodeString(const Code &line)
{
    constexpr size_t SIZE = 64;
    char buf[SIZE] = {'\0'};
    const char *op_name = GetOperationName(line.inst.operation);
    const uint16_t addr = line.addr;
    const uint8_t code = line.code;
    const uint8_t lo = line.lo;
    const uint8_t hi = line.hi;
    const uint16_t wd = line.wd;

    switch (line.inst.bytes) {
    case 1:
        snprintf(buf, SIZE, "%04X  %02X        %s", addr, code, op_name);
        break;

    case 2:
        snprintf(buf, SIZE, "%04X  %02X %02X     %s", addr, code, lo, op_name);
        break;

    case 3:
        snprintf(buf, SIZE, "%04X  %02X %02X %02X  %s", addr, code, lo, hi, op_name);
        break;
    }

    std::string result = buf;

    switch (line.inst.addr_mode) {
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

    case ZPG: case ZPX: case ZPY:
        snprintf(buf, SIZE, "$%02X", lo);
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

std::string GetMemoryString(const Code &line, const CPU &cpu)
{
    constexpr size_t SIZE = 32;
    char buf[SIZE] = {'\0'};
    const uint8_t lo = line.lo;
    const uint16_t wd = line.wd;

    CpuStatus stat;
    cpu.GetStatus(stat);
    const uint8_t  x = stat.x;
    const uint8_t  y = stat.y;

    switch (line.inst.addr_mode) {
    case IND:
        snprintf(buf, SIZE, " = %04X", cpu.GetAbsoluteIndirect(wd));
        break;

    case ABS:
        if (line.inst.operation != JMP && line.inst.operation != JSR)
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
