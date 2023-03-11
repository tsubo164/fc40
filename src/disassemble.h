#ifndef DISASSEMBLE_H
#define DISASSEMBLE_H

#include <unordered_map>
#include <cstdint>
#include <string>
#include "instruction.h"
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
    int FindCode(uint16_t addr) const;
    Code GetCode(int index) const;
    int GetCount() const;

    std::vector<Code> instructions_; 
    std::unordered_map<uint32_t,size_t> addr_map_;
};

Code DisassembleLine(const CPU &cpu, uint16_t addr);
extern void Disassemble(AssemblyCode &assem, const Cartridge &cart);
std::string GetCodeString(const Code &line);
std::string GetMemoryString(const Code &line, const CPU &cpu);

} // namespace

#endif // _H
