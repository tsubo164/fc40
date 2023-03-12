#ifndef DISASSEMBLE_H
#define DISASSEMBLE_H

#include "instruction.h"
#include "cpu.h"
#include <unordered_map>
#include <cstdint>
#include <vector>
#include <string>

namespace nes {

struct Code {
    Instruction instruction;
    uint16_t address;
    uint8_t opcode;
    uint8_t lo;
    uint8_t hi;
    uint16_t word;
};

class Assembly {
public:
    Assembly();
    ~Assembly();

    void DisassembleProgram(const CPU &cpu);

    int FindCode(uint16_t addr) const;
    Code GetCode(int index) const;
    int GetCount() const;

private:
    std::vector<Code> codes_;
    std::unordered_map<uint32_t,size_t> addr_to_code_;
};

Code DisassembleLine(const CPU &cpu, uint16_t addr);
std::string GetCodeString(const Code &code);
std::string GetMemoryString(const Code &code, const CPU &cpu);

} // namespace

#endif // _H
