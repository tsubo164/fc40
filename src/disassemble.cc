#include <cstdio>
#include "disassemble.h"

namespace nes {

void Disassemble(AssemblyCode &assem, const Cartridge &cart)
{
    uint32_t addr = 0;
    const uint32_t PROG_SIZE = cart.GetProgSize();

    for (;;) {
        Code line;
        const uint8_t code = cart.PeekProg(addr);
        const Instruction inst = Decode(code);

        line.inst = inst;
        line.addr = addr;
        line.code = code;
        line.lo   = cart.PeekProg(addr + 1);
        line.hi   = cart.PeekProg(addr + 2);
        line.wd   = (line.hi << 8) | line.lo;

        const uint32_t index_to_add = assem.instructions_.size();
        assem.instructions_.push_back(line);
        assem.addr_map_.insert({addr, index_to_add});

        addr += inst.bytes;

        if (addr > PROG_SIZE)
            break;
    }
}

void GetCodeLine(const AssemblyCode &assem,
        uint32_t physical_addr, uint16_t virtual_addr)
{
    const uint32_t index = assem.addr_map_.at(physical_addr);
    const Code line = assem.instructions_[index];

    switch (line.inst.bytes) {
    case 1:
        printf("%04X  %02X        %s\n", line.addr, line.code, GetMnemonic(line.code));
        break;

    case 2:
        printf("%04X  %02X %02X     %s\n", line.addr, line.code, line.lo,
                GetMnemonic(line.code));
        break;

    case 3:
        printf("%04X  %02X %02X %02X  %s\n", line.addr, line.code, line.lo, line.hi,
                GetMnemonic(line.code));
        break;
    }
}

void Disassemble2(AssemblyCode &assem, const Cartridge &cart)
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
        printf("%04X  %02X        %s\n", line.addr, line.code, GetMnemonic(line.code));
        break;

    case 2:
        printf("%04X  %02X %02X     %s\n", line.addr, line.code, line.lo,
                GetMnemonic(line.code));
        break;

    case 3:
        printf("%04X  %02X %02X %02X  %s\n", line.addr, line.code, line.lo, line.hi,
                GetMnemonic(line.code));
        break;
    }
}

} // namespace
