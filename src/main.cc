#include <iostream>
#include <string>
#include "nes.h"
#include "cartridge.h"
#include "debug.h"

using namespace nes;

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
        std::cerr << "mapper " << static_cast<int>(cart.MapperID())
                  << " is not supported." << std::endl;
        return -1;
    }

    nes.InsertCartridge(&cart);
    nes.PowerUp();

    if (log_mode)
        LogCpuStatus(&nes.cpu, 8980);
    else
        nes.PlayGame();

    nes.ShutDown();

    return 0;
}
