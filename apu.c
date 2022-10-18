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

static void calculate_target_period(struct sweep_unit *swp, uint16_t current_period);

void write_apu_status(struct APU *apu, uint8_t data)
{
    apu->pulse1.enabled = (data & 0x01);
    if (!apu->pulse1.enabled)
        apu->pulse1.length = 0;
}

void write_apu_square1_volume(struct APU *apu, uint8_t data)
{
    /* $4000 | DDlc.vvvv | Pulse 1 Duty cycle, length counter halt,
     * constant volume/envelope flag, and volume/envelope divider period
     *
     * The duty cycle is changed, but the sequencer's current position isn't affected.  */
    apu->pulse1.duty = (data >> 6) & 0x03;
    apu->pulse1.envelope.loop = (data >> 5) & 0x01;
    apu->pulse1.envelope.constant = (data >> 4) & 0x01;
    apu->pulse1.envelope.volume = (data & 0x0F);
}

void write_apu_square1_sweep(struct APU *apu, uint8_t data)
{
    /* $4001 | EPPP.NSSS | Pulse channel 1 sweep setup (write)
     * bit 7    | E--- ---- | Enabled flag
     * bits 6-4 | -PPP ---- | The divider's period is P + 1 half-frames
     * bit 3    | ---- N--- | Negate flag
     *          |           | 0: add to period, sweeping toward lower frequencies
     *          |           | 1: subtract from period, sweeping toward higher frequencies
     * bits 2-0 | ---- -SSS | Shift count (number of bits)
     * Side effects | Sets the reload flag */
    apu->pulse1.sweep.enabled = (data >> 7) & 0x01;
    /* The divider's period is set to P + 1 */
    apu->pulse1.sweep.period = ((data >> 4) & 0x07) + 1;
    apu->pulse1.sweep.negate = (data >> 3) & 0x01;
    apu->pulse1.sweep.shift = (data & 0x07);
    apu->pulse1.sweep.reload = 1;
}

void write_apu_square1_lo(struct APU *apu, uint8_t data)
{
    /* $4002 | LLLL.LLLL | Pulse 1 timer Low 8 bits */
    apu->pulse1.timer_period = (apu->pulse1.timer_period & 0xFF00) | data;
}

void write_apu_square1_hi(struct APU *apu, uint8_t data)
{
    /* $4003 | llll.lHHH | Pulse 1 length counter load and timer High 3 bits
     *
     * The sequencer is immediately restarted at the first value of the current sequence.
     * The envelope is also restarted. The period divider is not reset.  */
    apu->pulse1.length = length_table[data >> 3];
    apu->pulse1.timer_period = ((data & 0x07) << 8) | (apu->pulse1.timer_period & 0x00FF);
    /* XXX reset timer here? */
    apu->pulse1.timer = apu->pulse1.timer_period;
    apu->pulse1.sequence_pos = 0;
    apu->pulse1.envelope.start = 1;

    /* Whenever the current period changes for any reason, whether by $400x writes or
     * by sweep, the target period also changes. */
    calculate_target_period(&apu->pulse1.sweep, apu->pulse1.timer_period);
}

void reset_apu(struct APU *apu)
{
    apu->audio_time = 0.;
    apu->clock = 0;
    apu->cycle = 0;

    apu->pulse1.enabled = 0;
    apu->pulse1.length = 0;

    apu->pulse1.timer = 0;
    apu->pulse1.timer_period = 0;

    apu->pulse1.duty = 0;
    apu->pulse1.sequence_pos = 0;

    struct sweep_unit s = {0};
    apu->pulse1.sweep = s;

    struct envelope e = {0};
    apu->pulse1.envelope = e;
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

    if (sample == 0)
        return 0;

    if (pulse->enabled == 0)
        return 0;

    if (pulse->length == 0)
        return 0;

    if (pulse->timer_period < 8 || pulse->timer_period > 0x7FF)
        return 0;

    if (pulse->envelope.constant)
        return pulse->envelope.volume * 32767 / 16;
    else
        return pulse->envelope.decay * 32767 / 16;
    /*
    return sample * 32767 / 8;
    */
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

static void clock_length_counter(struct APU *apu)
{
    if (apu->pulse1.length > 0)
        apu->pulse1.length--;
}

static void calculate_target_period(struct sweep_unit *swp, uint16_t current_period)
{
    uint16_t change = current_period >> swp->shift;

    if (swp->negate) {
        swp->target_period = current_period - change;

        if (1 /* pulse1 */)
            swp->target_period--;
    }
    else {
        swp->target_period = current_period + change;
    }
}

static void clock_sweep(struct sweep_unit *swp, struct pulse_channel *pulse)
{
    if (swp->divider == 0 && swp->enabled
            && pulse->timer_period >= 8 && swp->target_period <= 0x07FF
            && swp->shift > 0
            /* && is_muting_pulse(swp) */) {
        pulse->timer_period = swp->target_period;
        calculate_target_period(&pulse->sweep, pulse->timer_period);
    }

    if (swp->divider == 0 || swp->reload) {
        swp->divider = swp->period;
        swp->reload = 0;
    }
    else {
        swp->divider--;
    }
}

static void clock_envelope(struct envelope *env)
{
    if (env->start == 0) {
        /* clock divider */
        if (env->divider == 0) {
            env->divider = env->volume;

            if (env->decay > 0)
                env->decay--;
            else if (env->loop)
                env->decay = 15;
        }
        else {
            env->divider--;
        }
    }
    else {
        env->start = 0;
        env->decay = 15;
        env->divider = env->volume;
    }
}

static void clock_sweeps(struct APU *apu)
{
    clock_sweep(&apu->pulse1.sweep, &apu->pulse1);
}

static void clock_envelopes(struct APU *apu)
{
    clock_envelope(&apu->pulse1.envelope);
}

static void clock_sequencer(struct APU *apu)
{
    switch (apu->cycle) {
    case 3729:
        clock_envelopes(apu);
        break;

    case 7457:
        clock_envelopes(apu);
        clock_length_counter(apu);
        clock_sweeps(apu);
        break;

    case 11186:
        clock_envelopes(apu);
        break;

    case 14915:
        clock_envelopes(apu);
        clock_length_counter(apu);
        clock_sweeps(apu);
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
