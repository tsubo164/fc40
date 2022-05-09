#include <stdio.h>
#include <string.h>

#include "framebuffer.h"
#include "cartridge.h"
#include "display.h"
#include "cpu.h"
#include "ppu.h"
#include "log.h"

static struct CPU cpu = {0};
static struct PPU ppu = {0};
static uint64_t clock = 0;

void update_frame(void);
void input_controller(uint8_t id, uint8_t input);
void log_cpu_status(struct CPU *cpu);

int main(int argc, char **argv)
{
    const int RESX = 256;
    const int RESY = 240;
    struct framebuffer *fbuf = NULL;
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

    cpu.cart = cart;
    cpu.ppu = &ppu;
    ppu.cart = cart;
    fbuf = new_framebuffer(RESX, RESY);
    ppu.fbuf = fbuf;

    reset(&cpu);

    if (log_mode)
        log_cpu_status(&cpu);
    else
        open_display(fbuf, update_frame, input_controller);

    free_framebuffer(fbuf);
    close_cartridge(cart);

    return 0;
}

void update_frame(void)
{
    do {
        clock_ppu(&ppu);

        if (clock++ % 3 == 0)
            clock_cpu(&cpu);

        if (is_nmi_generated(&ppu)) {
            clear_nmi(&ppu);
            nmi(&cpu);
        }

    } while (!is_frame_ready(&ppu));
}

void input_controller(uint8_t id, uint8_t input)
{
    set_controller_input(&cpu, 0, input);
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
