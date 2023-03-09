#ifndef DISASSEMBLE_H
#define DISASSEMBLE_H

#include <unordered_map>
#include <cstdint>
#include "cartridge.h"
#include "cpu.h"

namespace nes {

struct Code {
    Instruction inst;
    uint32_t addr;
    uint8_t code;
    uint8_t lo;
    uint8_t hi;
    uint16_t wd;
};

struct AssemblyCode {
    std::vector<Code> instructions_; 
    std::unordered_map<uint32_t,size_t> addr_map_;
};

extern void Disassemble(AssemblyCode &assem, const Cartridge &cart);
extern void GetCodeLine(const AssemblyCode &assem,
        uint32_t physical_addr, uint16_t virtual_addr);

extern void Disassemble2(AssemblyCode &assem, const Cartridge &cart);
extern void PrintLine(const Code &line);

} // namespace

#endif // _H
