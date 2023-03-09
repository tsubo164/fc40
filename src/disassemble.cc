#include <string>
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

    switch (line.inst.bytes) {
    case 1:
        sprintf(buf, "%04X  %02X        %s", line.addr, line.code,
                GetOperationName(line.inst.operation));
        break;

    case 2:
        sprintf(buf, "%04X  %02X %02X     %s", line.addr, line.code, line.lo,
                GetOperationName(line.inst.operation));
        break;

    case 3:
        sprintf(buf, "%04X  %02X %02X %02X  %s", line.addr, line.code, line.lo, line.hi,
                GetOperationName(line.inst.operation));
        break;
    }

    std::string result = buf;

    switch (line.inst.addr_mode) {
    case IMM:
        sprintf(buf, "#$%02X", line.lo);
        break;
    }

    return result;
}

} // namespace
