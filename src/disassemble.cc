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

void PrintLine(const Code &line)
{
    switch (line.inst.bytes) {
    case 1:
        printf("%04X  %02X        %s\n", line.addr, line.code,
                GetOperationName(line.inst.operation));
        break;

    case 2:
        printf("%04X  %02X %02X     %s\n", line.addr, line.code, line.lo,
                GetOperationName(line.inst.operation));
        break;

    case 3:
        printf("%04X  %02X %02X %02X  %s\n", line.addr, line.code, line.lo, line.hi,
                GetOperationName(line.inst.operation));
        break;
    }
}

std::string GetCodeString(const Code &line)
{
    static char buf[1024] = {'\0'};
    const char *op_name = GetOperationName(line.inst.operation);
    const uint16_t addr = line.addr;
    const uint8_t code = line.code;
    const uint8_t lo = line.lo;
    const uint8_t hi = line.hi;
    const uint16_t wd = line.wd;

    switch (line.inst.bytes) {
    case 1:
        sprintf(buf, "%04X  %02X        %s", addr, code, op_name);
        break;

    case 2:
        sprintf(buf, "%04X  %02X %02X     %s", addr, code, lo, op_name);
        break;

    case 3:
        sprintf(buf, "%04X  %02X %02X %02X  %s", addr, code, lo, hi, op_name);
        break;
    }

    std::string result = buf;
    buf[0] = '\0';

    switch (line.inst.addr_mode) {
    case ABS:
        sprintf(buf, "$%04X", wd);
        break;

    case ABX:
        sprintf(buf, "$%04X,X", wd);
        break;

    case ABY:
        sprintf(buf, "$%04X,Y", wd);
        break;

    case IND:
        sprintf(buf, "($%04X)", wd);
        break;

    case IZX:
        sprintf(buf, "($%02X,X)", lo);
        break;

    case IZY:
        sprintf(buf, "($%02X),Y", lo);
        break;

    case REL:
        sprintf(buf, "$%04X", (addr + 2) + static_cast<int8_t>(lo));
        break;

    case ZPG: case ZPX: case ZPY:
        sprintf(buf, "$%02X", lo);
        break;

    case IMM:
        sprintf(buf, "#$%02X", lo);
        break;

    case ACC:
        sprintf(buf, " A");
        break;

    case IMP:
    default:
        break;
    }

    if (buf[0])
        result += std::string(" ") + buf;

    return result;
}

} // namespace
