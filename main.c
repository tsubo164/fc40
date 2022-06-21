#include <stdio.h>
#include <string.h>

#include "nes.h"
#include "cartridge.h"
#include "display.h"
#include "debug.h"
#include "log.h"

struct NES nes = {{0}};

void log_cpu_status(struct CPU *cpu);

int main(int argc, char **argv)
{
    struct cartridge *cart = NULL;
    const char *filename = NULL;
    int log_mode = 0;

    if (argc == 3 && !strcmp(argv[1], "--log-mode")) {
        log_mode = 1;
        filename = argv[2];
    }
    else if (argc ==2) {
        filename = argv[1];
    }
    else {
        fprintf(stderr, "missing file name\n");
        return -1;
    }

    cart = open_cartridge(filename);
    if (!cart) {
        fprintf(stderr, "not a *.nes file\n");
        return -1;
    }

    insert_cartridge(&nes, cart);
    power_up_nes(&nes);

    if (log_mode) {
        log_cpu_status(&nes.cpu);
    }
    else {
        struct display disp;
        disp.nes = &nes;
        disp.fb = nes.fbuf;
        disp.pattern_table = nes.patt;
        disp.update_frame_func = update_frame;
        disp.input_controller_func = input_controller;
        disp.ppu = &nes.ppu;
        open_display(&disp);
    }

    shut_down_nes(&nes);
    close_cartridge(cart);

    return 0;
}

void log_cpu_status(struct CPU *cpu)
{
    uint16_t log_line = 0;
    cpu->pc = 0xC000;

    while (log_line < 8980) {
        if (cpu->cycles == 0) {
            print_cpu_log(cpu);
            log_line++;
        }
        clock_cpu(cpu);
    }
}
