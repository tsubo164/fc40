#include <stdio.h>

#include "framebuffer.h"
#include "cartridge.h"
#include "display.h"
#include "cpu.h"
#include "ppu.h"

static struct CPU cpu = {0};
static struct PPU ppu = {0};
static uint64_t clock = 0;

void update_frame(void);

int main(void)
{
    const int RESX = 256;
    const int RESY = 240;
    struct framebuffer *fbuf = NULL;
    struct cartridge *cart = open_cartridge("./sample1.nes");

    if (!cart) {
        fprintf(stderr, "not a *.nes file\n");
        return -1;
    }

    cpu.prog = cart->prog_rom;
    cpu.prog_size = cart->prog_size;
    ppu.char_rom = cart->char_rom;
    ppu.char_size = cart->char_size;
    fbuf = new_framebuffer(RESX, RESY);
    ppu.fbuf = fbuf;

    reset(&cpu);

    open_display(fbuf, update_frame);

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

    } while (!is_frame_ready(&ppu));
}
