#include <iostream>
#include <string>
#include "nes.h"
#include "cartridge.h"
#include "debug.h"
#include "log.h"

using namespace nes;

void Disassemble(CPU *cpu);

int main(int argc, char **argv)
{
    NES nes;
    Cartridge cart;
    const char *filename = nullptr;
    int log_mode = 0;

    if (argc == 3 && std::string(argv[1]) == "--log-mode") {
        log_mode = 1;
        filename = argv[2];
    }
    else if (argc ==2) {
        filename = argv[1];
    }
    else {
        std::cerr << "missing file name" << std::endl;
        return -1;
    }

    if (!cart.Open(filename)) {
        std::cerr << "not a *.nes file" << std::endl;
        return -1;
    }

    if (!cart.IsMapperSupported()) {
        std::cerr << "mapper " << static_cast<int>(cart.GetMapperID())
                  << " is not supported." << std::endl;
        return -1;
    }

    nes.InsertCartridge(&cart);
    nes.PowerUp();

    if (0) {
        Disassemble(&nes.cpu);
        return 0;
    }

    if (log_mode) {
        LogCpuStatus(&nes.cpu, 8980);
    }
    else {
        printf("iNES Mapper : %4d\n", cart.GetMapperID());
        printf("PRG Size    : %4ld KB\n", cart.GetProgSize() / 1024);
        printf("CHR Size    : %4ld KB\n", cart.GetCharSize() / 1024);

        nes.PlayGame();
    }

    nes.ShutDown();

    return 0;
}

void Disassemble(CPU *cpu)
{
    cpu->SetPC(0x0000);

    for (;;) {
        PrintCpuStatus(cpu);

        int bytes = 1;
        CpuStatus stat;
        cpu->GetStatus(stat);
        const std::string mode_name = stat.mode_name;

        if (mode_name == "IND") {
            bytes = 3;
        }
        else if (mode_name == "ABS") {
            bytes = 3;
        }
        else if (mode_name == "ABX") {
            bytes = 3;
        }
        else if (mode_name == "ABY") {
            bytes = 3;
        }
        else if (mode_name == "IZX") {
            bytes = 2;
        }
        else if (mode_name == "IZY") {
            bytes = 2;
        }
        else if (mode_name == "ZPX") {
            bytes = 2;
        }
        else if (mode_name == "ZPY") {
            bytes = 2;
        }
        else if (mode_name == "REL") {
            bytes = 3;
        }
        else if (mode_name == "IMM") {
            bytes = 2;
        }
        else if (mode_name == "ZPG") {
            bytes = 2;
        }
        else if (mode_name == "ACC") {
            bytes = 1;
        }
        else if (mode_name == "IMP") {
            bytes = 1;
        }

        if (cpu->GetPC() + bytes > 0xFFFF)
            break;

        cpu->SetPC(cpu->GetPC() + bytes);
    }
}
