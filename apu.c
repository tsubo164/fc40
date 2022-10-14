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

    /*
    $4000 | DDlc.vvvv | Pulse 1 Duty cycle, length counter halt,
    constant volume/envelope flag, and volume/envelope divider period
    */
    /*
    The duty cycle is changed, but the sequencer's current position isn't affected.
    */
    apu->pulse1.duty = (data >> 6) & 0x03;
    apu->pulse1.length_halt = (data >> 5) & 0x01;
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

    /*
    $4002 | LLLL.LLLL | Pulse 1 timer Low 8 bits
    */
    apu->pulse1.timer_period = (apu->pulse1.timer_period & 0xFF00) | data;
}

void write_apu_square1_hi(struct APU *apu, uint8_t data)
{
    apu->pulse1_seq_reload = ((data & 0x07) << 8) | (apu->pulse1_seq_reload & 0x00FF);
    apu->pulse1_seq_timer = apu->pulse1_seq_reload;
    apu->pulse1_seq_sequence = apu->pulse1_seq_new_sequence;
    apu->pulse1_length_counter = length_table[(data & 0xF8) >> 3];
    apu->pulse1_env_start = 1;

    /*
    $4003 | llll.lHHH | Pulse 1 length counter load and timer High 3 bits
    */
    /*
    The sequencer is immediately restarted at the first value of the current sequence.
    The envelope is also restarted. The period divider is not reset.
    */
    apu->pulse1.length = length_table[data >> 3];

    apu->pulse1.timer_period = ((data & 0x07) << 8) | (apu->pulse1.timer_period & 0x00FF);
    /* XXX reset timer here??
    apu->pulse1.timer = apu->pulse1.timer_period;
    */

    apu->pulse1.sequence_pos = 0;
}

void reset_apu(struct APU *apu)
{
    apu->audio_time = 0.;
    apu->clock = 0;
    apu->cycle = 0;

    apu->pulse1.enabled = 0;

    apu->pulse1.length = 0;
    apu->pulse1.length_halt = 0;

    apu->pulse1.timer = 0;
    apu->pulse1.timer_period = 0;

    apu->pulse1.duty = 0;
    apu->pulse1.sequence_pos = 0;
}

static uint16_t sample_pulse(struct pulse_channel *pulse)
{
    const static uint8_t sequence_table[][8] = {
        {0, 1, 0, 0, 0, 0, 0, 0}, /* (12.5%) */
        {0, 1, 1, 0, 0, 0, 0, 0}, /* (25%) */
        {0, 1, 1, 1, 1, 0, 0, 0}, /* (50%) */
        {1, 0, 0, 1, 1, 1, 1, 1}  /* (25% negated) */
    };

    const uint16_t sample = sequence_table[pulse->duty][pulse->sequence_pos];

    if (pulse->enabled == 0)
        return 0;

    if (pulse->length == 0)
        return 0;

    if (pulse->timer_period < 8 || pulse->timer_period > 0x7FF)
        return 0;

    return sample * 32767 / 8;
}

static void update_timer(struct APU *apu)
{
    if (apu->pulse1.timer == 0) {
        apu->pulse1.timer = apu->pulse1.timer_period;
        apu->pulse1.sequence_pos = (apu->pulse1.sequence_pos + 1) % 8;
    }
    else {
        apu->pulse1.timer--;
    }
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

    if (apu->clock % 2 == 0)
        update_timer(apu);

    apu->audio_time += APU_TIME_STEP;

    if (apu->audio_time > AUDIO_SAMPLE_STEP) {
        /* generate a sample */
        apu->audio_time -= AUDIO_SAMPLE_STEP;

        int16_t sample = 0;
        if (apu->pulse1.length > 0) {
            if (0) {
                /* sin wave */
                static int64_t c = 0;
                sample = 32767 * sin(2 * M_PI * 440 * c/44100);
                c++;
            }
            else {
                sample = sample_pulse(&apu->pulse1);
            }
        }
        push_sample(sample);
        /*
        push_sample__(sample);
        */
    }
}
