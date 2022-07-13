#include <stdlib.h>

#include "nes.h"
#include "framebuffer.h"
#include "display.h"
#include "debug.h"

#include <al.h>
#include <alc.h>
#include <math.h>
#include <stdio.h>

void play_sound(struct NES *nes)
{
    const int SAMPLINGRATE = 44100;
    signed short *wav_data = NULL;
    int i;

    ALCdevice *device = alcOpenDevice(NULL);
    ALCcontext *context = alcCreateContext(device, NULL);
    alcMakeContextCurrent(context);

    ALuint buffer;
    ALuint source;
    alGenBuffers(1, &buffer);
    alGenSources(1, &source);

    wav_data = malloc(sizeof(signed short) * SAMPLINGRATE);
    for (i = 0; i < SAMPLINGRATE; i++)
        wav_data[i] = 32767 * sin(2 * M_PI*i * 440 / SAMPLINGRATE);

    alBufferData(buffer, AL_FORMAT_MONO16, &wav_data[0],
            SAMPLINGRATE * sizeof(signed short), SAMPLINGRATE);
    alSourcei(source, AL_BUFFER, buffer);
    alSourcei(source, AL_LOOPING, AL_TRUE);
    alSourcePlay(source);

    for (i = 0; i < 1000000; i++) {
        /*
        printf("%d: Hello OpenAL!\n", i);
        */
    }

    alSourceStop(source);

    free(wav_data);
    alDeleteBuffers(1, &buffer);
    alDeleteSources(1, &source);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);
}

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

    play_sound(nes);

    disp.nes = nes;
    disp.fb = nes->fbuf;
    disp.pattern_table = nes->patt;
    disp.update_frame_func = update_frame;
    disp.input_controller_func = input_controller;
    disp.ppu = &nes->ppu;

    open_display(&disp);
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
