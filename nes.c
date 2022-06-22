#include <stdlib.h>

#include "nes.h"
#include "framebuffer.h"
#include "display.h"
#include "debug.h"

void power_up_nes(struct NES *nes)
{
    const int RESX = 256;
    const int RESY = 240;

    /* dma */
    nes->dma_wait = 1;
    nes->dma_addr = 0x00;
    nes->dma_page = 0x00;
    nes->dma_data = 0x00;

    /* framebuffer */
    nes->fbuf = new_framebuffer(RESX, RESY);
    nes->ppu.fbuf = nes->fbuf;

    /* pattern table */
    nes->patt = new_framebuffer(16 * 8 * 2, 16 * 8);
    load_pattern_table(nes->patt, nes->cart);

    /* CPU and PPU */
    nes->cpu.ppu = &nes->ppu;
    power_up_cpu(&nes->cpu);
    power_up_ppu(&nes->ppu);
}

void shut_down_nes(struct NES *nes)
{
    free_framebuffer(nes->fbuf);
    free_framebuffer(nes->patt);

    nes->fbuf = NULL;
    nes->patt = NULL;
}

void insert_cartridge(struct NES *nes, struct cartridge *cart)
{
    nes->cart = cart;
    nes->cpu.cart = nes->cart;
    nes->ppu.cart = nes->cart;
}

void play_game(struct NES *nes)
{
    struct display disp = {0};

    disp.nes = nes;
    disp.fb = nes->fbuf;
    disp.pattern_table = nes->patt;
    disp.update_frame_func = update_frame;
    disp.input_controller_func = input_controller;
    disp.ppu = &nes->ppu;

    open_display(&disp);
}

static void clock_dma(struct NES *nes)
{
    if (nes->dma_wait) {
        if (nes->clock % 2 == 1) {
            nes->dma_wait = 0;
            nes->dma_addr = 0x00;
            nes->dma_page = get_dma_page(&nes->cpu);
        }
        /* idle for this cpu cycle */
        return;
    }

    if (nes->clock % 2 == 0) {
        /* read */
        nes->dma_data = read_cpu_data(&nes->cpu, (nes->dma_page << 8) | nes->dma_addr);
    }
    else {
        /* write */
        write_dma_sprite(&nes->ppu, nes->dma_addr, nes->dma_data);
        nes->dma_addr++;

        if (nes->dma_addr == 0x00) {
            nes->dma_wait = 1;
            nes->dma_page = 0x00;
            nes->dma_addr = 0x00;
            resume(&nes->cpu);
        }
    }
}

void update_frame(struct NES *nes)
{
    do {
        clock_ppu(&nes->ppu);

        if (nes->clock++ % 3 == 0) {
            if (is_suspended(&nes->cpu))
                clock_dma(nes);
            else
                clock_cpu(&nes->cpu);
        }

        if (is_nmi_generated(&nes->ppu)) {
            clear_nmi(&nes->ppu);
            nmi(&nes->cpu);
        }

    } while (!is_frame_ready(&nes->ppu));
}

void input_controller(struct NES *nes, uint8_t id, uint8_t input)
{
    set_controller_input(&nes->cpu, 0, input);
}
