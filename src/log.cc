#include <cstdio>
#include <string>
#include "log.h"
#include "cpu.h"

namespace nes {

static uint16_t abs_indirect(const struct CPU *cpu, uint16_t abs)
{
    if ((abs & 0x00FF) == 0x00FF) {
        // emulate page boundary hardware bug
        return (cpu->PeekData(abs & 0xFF00) << 8) | cpu->PeekData(abs);
    }
    else {
        // normal behavior
        const uint16_t lo = cpu->PeekData(abs);
        const uint16_t hi = cpu->PeekData(abs + 1);

        return (hi << 8) | lo;
    }
}

static uint16_t zp_indirect(const struct CPU *cpu, uint8_t zp)
{
    const uint16_t lo = cpu->PeekData(zp & 0xFF);
    const uint16_t hi = cpu->PeekData((zp + 1) & 0xFF);

    return (hi << 8) | lo;
}

void PrintCpuStatus(const struct CPU *cpu)
{
    CpuStatus stat;
    cpu->GetStatus(stat);

    const uint16_t pc = stat.pc;
    const uint8_t  a = stat.a;
    const uint8_t  x = stat.x;
    const uint8_t  y = stat.y;
    const uint8_t  p = stat.p;
    const uint8_t  s = stat.s;
    const uint8_t  code = stat.code;
    const uint8_t  lo   = stat.lo;
    const uint8_t  hi   = stat.hi;
    const uint16_t wd   = stat.wd;
    const char *inst_name = stat.inst_name;
    const std::string mode_name = stat.mode_name;

    int N = 0, n = 0;

    printf("%04X  %02X %n", pc, code, &n);
    N += n;

    if (mode_name == "IND") {
        printf("%02X %02X  %s ($%04X) = %04X%n",
                lo, hi, inst_name, wd, abs_indirect(cpu, wd), &n);
        N += n;
    }
    else if (mode_name == "ABS") {
        printf("%02X %02X  %s $%04X%n", lo, hi, inst_name, wd, &n);
        N += n;

        const std::string inst = inst_name;
        if (inst != "JMP" && inst != "JSR") {
            printf(" = %02X%n", cpu->PeekData(wd), &n);
            N += n;
        }
    }
    else if (mode_name == "ABX") {
        printf("%02X %02X  %s $%04X,X @ %04X = %02X%n",
                lo, hi, inst_name, wd, wd + x, cpu->PeekData(wd + x), &n);
        N += n;
    }
    else if (mode_name == "ABY") {
        printf("%02X %02X  %s $%04X,Y @ %04X = %02X%n",
                lo, hi, inst_name, wd, (wd + y) & 0xFFFF,
                cpu->PeekData((wd + y) & 0xFFFF), &n);
        N += n;
    }
    else if (mode_name == "IZX") {
        const uint16_t addr = zp_indirect(cpu, lo + x);
        printf("%02X     %s ($%02X,X) @ %02X = %04X = %02X%n",
                lo, inst_name, lo, (lo + x) & 0xFF, addr, cpu->PeekData(addr), &n);
        N += n;
    }
    else if (mode_name == "IZY") {
        const uint16_t addr = zp_indirect(cpu, lo);
        printf("%02X     %s ($%02X),Y = %04X @ %04X = %02X%n",
                lo, inst_name, lo, addr, (addr + y) & 0xFFFF, cpu->PeekData(addr + y), &n);
        N += n;
    }
    else if (mode_name == "ZPX") {
        printf("%02X     %s $%02X,X @ %02X = %02X%n",
                lo, inst_name, lo, (lo + x) & 0xFF, cpu->PeekData((lo + x) & 0xFF), &n);
        N += n;
    }
    else if (mode_name == "ZPY") {
        printf("%02X     %s $%02X,Y @ %02X = %02X%n",
                lo, inst_name, lo, (lo + y) & 0xFF, cpu->PeekData((lo + y) & 0xFF), &n);
        N += n;
    }
    else if (mode_name == "REL") {
        printf("%02X     %s $%04X%n", lo, inst_name, (pc + 2) + static_cast<int8_t>(lo), &n);
        N += n;
    }
    else if (mode_name == "IMM") {
        printf("%02X     %s #$%02X%n", lo, inst_name, lo, &n);
        N += n;
    }
    else if (mode_name == "ZPG") {
        printf("%02X     %s $%02X = %02X %n",
                lo, inst_name, lo, cpu->PeekData(lo), &n);
        N += n;
    }
    else if (mode_name == "ACC") {
        printf("       %s A%n", inst_name, &n);
        N += n;
    }
    else if (mode_name == "IMP") {
        printf("       %s%n", inst_name, &n);
        N += n;
    }

    printf("%*s", 48 - N, " ");
    printf("A:%02X X:%02X Y:%02X P:%02X SP:%02X", a, x, y, p, s);
    printf("\n");
}

} // namespace
