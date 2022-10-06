#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "apu.h"
#include "sound.h"

static uint8_t length_table[] = {
     10, 254,  20,   2,  40,   4,  80,   6,
    160,   8,  60,  10,  14,  12,  26,  14,
     12,  16,  24,  18,  48,  20,  96,  22,
    192,  24,  72,  26,  16,  28,  32,  30
};

void write_apu_status(struct APU *apu, uint8_t data)
{
    apu->pulse1.enabled = (data & 0x01);
    if (!apu->pulse1.enabled)
        apu->pulse1.length = 0;
}

void write_apu_pulse1_length(struct APU *apu, uint8_t data)
{
    apu->pulse1.length = length_table[data >> 3];
}

void write_apu_square1_volume(struct APU *apu, uint8_t data)
{
    static uint8_t sequence_table[] = { 0x40,  0x60,  0x78,  0x9F};
    static float duty_table[]       = {0.125, 0.250, 0.500, 0.750};
    const int duty = ((data & 0xC0) >> 6);

    apu->pulse1_seq_new_sequence = sequence_table[duty];
    apu->pulse1_osc_dutycycle = duty_table[duty];

    apu->pulse1_seq_sequence = apu->pulse1_seq_new_sequence;
    apu->pulse1_halt = (data & 0x20);
    apu->pulse1_env_volume = (data & 0x0F);
    apu->pulse1_env_disable = (data & 0x10);
}

void write_apu_square1_sweep(struct APU *apu, uint8_t data)
{
    apu->pulse1_sweep_enabled = (data & 0x80) >> 7;
    apu->pulse1_sweep_period = (data & 0x70) >> 4;
    apu->pulse1_sweep_down = (data & 0x08) >> 3;
    apu->pulse1_sweep_shift = data & 0x07;
    apu->pulse1_sweep_reload = 1;
}

void write_apu_square1_lo(struct APU *apu, uint8_t data)
{
    apu->pulse1_seq_reload = (apu->pulse1_seq_reload & 0xFF00) | data;
}

void write_apu_square1_hi(struct APU *apu, uint8_t data)
{
    apu->pulse1_seq_reload = ((data & 0x07) << 8) | (apu->pulse1_seq_reload & 0x00FF);
    apu->pulse1_seq_timer = apu->pulse1_seq_reload;
    apu->pulse1_seq_sequence = apu->pulse1_seq_new_sequence;
    apu->pulse1_length_counter = length_table[(data & 0xF8) >> 3];
    apu->pulse1_env_start = 1;
}

void reset_apu(struct APU *apu)
{
    apu->audio_time = 0.;
    apu->clock = 0;
    apu->cycle = 0;
    apu->pulse1.enabled = 0;
    apu->pulse1.length = 0;
}

static void clock_frame_counter(struct APU *apu)
{
    if (apu->pulse1.length > 0)
        apu->pulse1.length--;
}

static void clock_sequencer(struct APU *apu)
{
    switch (apu->cycle) {
    case 3729:
        break;

    case 7457:
        clock_frame_counter(apu);
        break;

    case 14915:
        clock_frame_counter(apu);
        break;

    default:
        break;
    }

    if (apu->cycle == 14915)
        apu->cycle = 0;
    else
        apu->cycle++;
}

#define CPU_CLOCK_FREQ 1789773
#define APU_TIME_STEP (1. / CPU_CLOCK_FREQ)
#define AUDIO_SAMPLE_STEP (1. / 44100)

void clock_apu(struct APU *apu)
{
    /* apu clocked every other cpu cycles */
    if (apu->clock++ % 2 == 0)
        clock_sequencer(apu);

    apu->audio_time += APU_TIME_STEP;

    if (apu->audio_time > AUDIO_SAMPLE_STEP) {
        /* generate a sample */
        apu->audio_time -= AUDIO_SAMPLE_STEP;

        /*
        if (apu->pulse1.length > 0)
            apu->pulse1.length--;
        */
        int16_t sample = 0;
        if (apu->pulse1.length > 0) {
            /* sin wave */
            static int64_t c = 0;
            sample = 32767 * sin(2 * M_PI * 440 * c/44100);
            c++;
        }
        push_sample(sample);
        /*
        push_sample__(sample);
        */
    }
}
