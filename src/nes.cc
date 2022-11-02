#include <stdlib.h>

#include "nes.h"
#include "framebuffer.h"
#include "display.h"
#include "sound.h"
#include "debug.h"

#define AUDIO_DELAY_FRAME 2

namespace nes {

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
    nes->fbuf.Resize(RESX, RESY);
    nes->ppu.fbuf = &nes->fbuf;

    /* pattern table */
    nes->patt.Resize(16 * 8 * 2, 16 * 8);
    LoadPatternTable(nes->patt, nes->cart);

    /* CPU and PPU */
    nes->cpu.ppu = &nes->ppu;
    power_up_cpu(&nes->cpu);
    power_up_ppu(&nes->ppu);
}

void shut_down_nes(struct NES *nes)
{
}

void insert_cartridge(struct NES *nes, struct cartridge *cart)
{
    nes->cart = cart;
    nes->cpu.cart = nes->cart;
    nes->ppu.cart = nes->cart;
}

static void send_initial_samples(void)
{
    const int N = AUDIO_DELAY_FRAME * 44100 / 60;
    int i;
    for (i = 0; i < N; i++)
        PushSample(0.);
    SendSamples();
}

void play_game(struct NES *nes)
{
    Display disp = {0};

    InitSound();
    send_initial_samples();

    disp.nes = nes;
    disp.fb = &nes->fbuf;
    disp.pattern_table = &nes->patt;
    disp.update_frame_func = update_frame;
    disp.input_controller_func = input_controller;
    disp.ppu = &nes->ppu;

    //open_display(&disp);
    const int err = disp.Open();
    if (err) {
    }

    FinishSound();
}

void push_reset_button(struct NES *nes)
{
    reset_cpu(&nes->cpu);
    reset_ppu(&nes->ppu);
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
    static uint64_t frame = 0;
    if (frame % AUDIO_DELAY_FRAME == 0)
        PlaySamples();

    do {
        clock_ppu(&nes->ppu);

        if (nes->clock % 3 == 0) {
            if (is_suspended(&nes->cpu))
                clock_dma(nes);
            else
                clock_cpu(&nes->cpu);

            clock_apu(&nes->cpu.apu);
        }

        if (is_nmi_generated(&nes->ppu)) {
            clear_nmi(&nes->ppu);
            nmi(&nes->cpu);
        }

        if (is_irq_generated(&nes->cpu.apu)) {
            clear_irq(&nes->cpu.apu);
            irq(&nes->cpu);
        }

        nes->clock++;

    } while (!is_frame_ready(&nes->ppu));

    if (frame % AUDIO_DELAY_FRAME == 0)
        SendSamples();
    frame++;
}

void input_controller(struct NES *nes, uint8_t id, uint8_t input)
{
    set_controller_input(&nes->cpu, 0, input);
}

} // namespace
