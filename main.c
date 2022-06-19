#include <stdio.h>
#include <string.h>

#include "framebuffer.h"
#include "cartridge.h"
#include "display.h"
#include "cpu.h"
#include "ppu.h"
#include "log.h"
#include "debug.h"

static struct CPU cpu = {0};
static struct PPU ppu = {0};
static uint64_t clock = 0;

static int dma_wait = 1;
static uint8_t dma_addr = 0x00;
static uint8_t dma_page = 0x00;
static uint8_t dma_data = 0x00;

void update_frame(void);
void input_controller(uint8_t id, uint8_t input);
void log_cpu_status(struct CPU *cpu);

int main(int argc, char **argv)
{
    const int RESX = 256;
    const int RESY = 240;
    struct framebuffer *fbuf = NULL;
    struct framebuffer *patt = NULL;
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

    power_up_cpu(&cpu);
    power_up_ppu(&ppu);

    /* pattern table */
    patt = new_framebuffer(16 * 8 * 2, 16 * 8);
    load_pattern_table(patt, cart);

    if (log_mode) {
        log_cpu_status(&cpu);
    }
    else {
        struct display disp;
        disp.fb = fbuf;
        disp.pattern_table = patt;
        disp.update_frame_func = update_frame;
        disp.input_controller_func = input_controller;
        disp.ppu = &ppu;
        open_display(&disp);
    }

    free_framebuffer(fbuf);
    close_cartridge(cart);

    return 0;
}

static void clock_dma(void)
{
    if (dma_wait) {
        if (clock % 2 == 1) {
            dma_wait = 0;
            dma_addr = 0x00;
            dma_page = get_dma_page(&cpu);
        }
        /* idle for this cpu cycle */
        return;
    }

    if (clock % 2 == 0) {
        /* read */
        dma_data = read_cpu_data(&cpu, (dma_page << 8) | dma_addr);
    }
    else {
        /* write */
        write_dma_sprite(&ppu, dma_addr, dma_data);
        dma_addr++;

        if (dma_addr == 0x00) {
            dma_wait = 1;
            dma_page = 0x00;
            dma_addr = 0x00;
            resume(&cpu);
        }
    }
}

void update_frame(void)
{
    do {
        clock_ppu(&ppu);

        if (clock++ % 3 == 0) {
            if (is_suspended(&cpu))
                clock_dma();
            else
                clock_cpu(&cpu);
        }

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
