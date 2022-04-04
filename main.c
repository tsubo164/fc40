#include <stdio.h>
#include <string.h>

#include "framebuffer.h"
#include "cartridge.h"
#include "display.h"
#include "cpu.h"
#include "ppu.h"

static struct CPU cpu = {0};
static struct PPU ppu = {0};
static uint64_t clock = 0;

void update_frame(void);

int main(int argc, char **argv)
{
    const int RESX = 256;
    const int RESY = 240;
    struct framebuffer *fbuf = NULL;
    struct cartridge *cart = NULL;
    const char *filename = NULL;

    if (argc == 3 && !strcmp(argv[1], "--log-mode")) {
        cpu.log_mode = 1;
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
    ppu.char_rom = cart->char_rom;
    ppu.char_size = cart->char_size;
    fbuf = new_framebuffer(RESX, RESY);
    ppu.fbuf = fbuf;

    reset(&cpu);

    if (cpu.log_mode) {
        cpu.reg.pc = 0xC000;
        while (cpu.log_line < 5003)
            clock_cpu(&cpu);
    }
    else {
        open_display(fbuf, update_frame);
    }

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
