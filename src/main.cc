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
    bool test_mode = false;

    if (argc == 3 && std::string(argv[1]) == "--test-mode") {
        test_mode = true;
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

    if (test_mode) {
        LogCpuStatus(nes, 8991);
    }
    else {
        printf("iNES Mapper     : %4d\n", cart.GetMapperID());
        printf("PRG Size        : %4ld KB\n", cart.GetProgSize() / 1024);
        printf("CHR Size        : %4ld KB\n", cart.GetCharSize() / 1024);
        printf("Battery Present : %4s\n", cart.HasBattery() ? "Yes" : "No");

        nes.PlayGame();
    }

    nes.ShutDown();

    return 0;
}
