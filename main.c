#include <stdio.h>
#include <string.h>

#include "framebuffer.h"
#include "cartridge.h"
#include "display.h"
#include "cpu.h"
#include "ppu.h"
#include "log.h"
#include "debug.h"

struct NES {
    struct CPU cpu;
    struct PPU ppu;
    uint64_t clock;

    int dma_wait;
    uint8_t dma_addr;
    uint8_t dma_page;
    uint8_t dma_data;

    struct framebuffer *fbuf;
    struct framebuffer *patt;
    struct cartridge *cart;
};

struct NES nes = {{0}};

void power_up_nes(struct NES *nes)
{
    const int RESX = 256;
    const int RESY = 240;

    nes->dma_wait = 1;

    nes->fbuf = new_framebuffer(RESX, RESY);
    nes->ppu.fbuf = nes->fbuf;

    nes->patt = new_framebuffer(16 * 8 * 2, 16 * 8);
}

void update_frame(void);
void input_controller(uint8_t id, uint8_t input);
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

    nes.cpu.cart = cart;
    nes.cpu.ppu = &nes.ppu;
    nes.ppu.cart = cart;

    power_up_nes(&nes);
    power_up_cpu(&nes.cpu);
    power_up_ppu(&nes.ppu);

    /* pattern table */
    load_pattern_table(nes.patt, cart);

    if (log_mode) {
        log_cpu_status(&nes.cpu);
    }
    else {
        struct display disp;
        disp.fb = nes.fbuf;
        disp.pattern_table = nes.patt;
        disp.update_frame_func = update_frame;
        disp.input_controller_func = input_controller;
        disp.ppu = &nes.ppu;
        open_display(&disp);
    }

    free_framebuffer(nes.fbuf);
    free_framebuffer(nes.patt);
    close_cartridge(cart);

    return 0;
}

static void clock_dma(void)
{
    if (nes.dma_wait) {
        if (nes.clock % 2 == 1) {
            nes.dma_wait = 0;
            nes.dma_addr = 0x00;
            nes.dma_page = get_dma_page(&nes.cpu);
        }
        /* idle for this cpu cycle */
        return;
    }

    if (nes.clock % 2 == 0) {
        /* read */
        nes.dma_data = read_cpu_data(&nes.cpu, (nes.dma_page << 8) | nes.dma_addr);
    }
    else {
        /* write */
        write_dma_sprite(&nes.ppu, nes.dma_addr, nes.dma_data);
        nes.dma_addr++;

        if (nes.dma_addr == 0x00) {
            nes.dma_wait = 1;
            nes.dma_page = 0x00;
            nes.dma_addr = 0x00;
            resume(&nes.cpu);
        }
    }
}

void update_frame(void)
{
    do {
        clock_ppu(&nes.ppu);

        if (nes.clock++ % 3 == 0) {
            if (is_suspended(&nes.cpu))
                clock_dma();
            else
                clock_cpu(&nes.cpu);
        }

        if (is_nmi_generated(&nes.ppu)) {
            clear_nmi(&nes.ppu);
            nmi(&nes.cpu);
        }

    } while (!is_frame_ready(&nes.ppu));
}

void input_controller(uint8_t id, uint8_t input)
{
    set_controller_input(&nes.cpu, 0, input);
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
