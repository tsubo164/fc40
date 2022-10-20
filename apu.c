#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "apu.h"
#include "sound.h"

static void calculate_target_period(struct pulse_channel *pulse);

static void write_pulse_volume(struct pulse_channel *pulse, uint8_t data)
{
    /* $4000/$4004 | DDlc.vvvv | Pulse 1/2 Duty cycle, length counter halt,
     * constant volume/envelope flag, and volume/envelope divider period
     *
     * The duty cycle is changed, but the sequencer's current position isn't affected.  */
    pulse->duty = (data >> 6) & 0x03;
    pulse->envelope.loop = (data >> 5) & 0x01;
    pulse->envelope.constant = (data >> 4) & 0x01;
    pulse->envelope.volume = (data & 0x0F);
}

static void write_pulse_sweep(struct pulse_channel *pulse, uint8_t data)
{
    /* $4001/$4005 | EPPP.NSSS | Pulse channel 1/2 sweep setup (write)
     * bit 7    | E--- ---- | Enabled flag
     * bits 6-4 | -PPP ---- | The divider's period is P + 1 half-frames
     * bit 3    | ---- N--- | Negate flag
     *          |           | 0: add to period, sweeping toward lower frequencies
     *          |           | 1: subtract from period, sweeping toward higher frequencies
     * bits 2-0 | ---- -SSS | Shift count (number of bits)
     * Side effects | Sets the reload flag */
    pulse->sweep.enabled = (data >> 7) & 0x01;
    /* The divider's period is set to P + 1 */
    pulse->sweep.period = ((data >> 4) & 0x07) + 1;
    pulse->sweep.negate = (data >> 3) & 0x01;
    pulse->sweep.shift = (data & 0x07);
    pulse->sweep.reload = 1;
}

static void write_pulse_lo(struct pulse_channel *pulse, uint8_t data)
{
    /* $4002/$4006 | LLLL.LLLL | Pulse 1/2 timer Low 8 bits */
    pulse->timer_period = (pulse->timer_period & 0xFF00) | data;
    calculate_target_period(pulse);
}

static void write_pulse_hi(struct pulse_channel *pulse, uint8_t data)
{
    static uint8_t length_table[] = {
         10, 254,  20,   2,  40,   4,  80,   6,
        160,   8,  60,  10,  14,  12,  26,  14,
         12,  16,  24,  18,  48,  20,  96,  22,
        192,  24,  72,  26,  16,  28,  32,  30
    };

    /* $4003/$4007 | llll.lHHH | Pulse 1/2 length counter load and timer High 3 bits
     *
     * The sequencer is immediately restarted at the first value of the current sequence.
     * The envelope is also restarted. The period divider is not reset.  */
    pulse->length = length_table[data >> 3];
    pulse->timer_period = ((data & 0x07) << 8) | (pulse->timer_period & 0x00FF);
    /* XXX reset timer here? */
    pulse->timer = pulse->timer_period;
    pulse->sequence_pos = 0;
    pulse->envelope.start = 1;

    /* Whenever the current period changes for any reason, whether by $400x writes or
     * by sweep, the target period also changes. */
    calculate_target_period(pulse);
}

void write_apu_status(struct APU *apu, uint8_t data)
{

    /* $4015 write | ---D NT21 | Enable DMC (D), noise (N), triangle (T),
     * and pulse channels (2/1) */
    apu->pulse1.enabled = (data & 0x01);
    if (!apu->pulse1.enabled)
        apu->pulse1.length = 0;

    apu->pulse2.enabled = (data >> 1) & 0x01;
    if (!apu->pulse2.enabled)
        apu->pulse2.length = 0;
}

void write_apu_square1_volume(struct APU *apu, uint8_t data)
{
    write_pulse_volume(&apu->pulse1, data);
}

void write_apu_square1_sweep(struct APU *apu, uint8_t data)
{
    write_pulse_sweep(&apu->pulse1, data);
}

void write_apu_square1_lo(struct APU *apu, uint8_t data)
{
    write_pulse_lo(&apu->pulse1, data);
}

void write_apu_square1_hi(struct APU *apu, uint8_t data)
{
    write_pulse_hi(&apu->pulse1, data);
}

void write_apu_square2_volume(struct APU *apu, uint8_t data)
{
    write_pulse_volume(&apu->pulse2, data);
}

void write_apu_square2_sweep(struct APU *apu, uint8_t data)
{
    write_pulse_sweep(&apu->pulse2, data);
}

void write_apu_square2_lo(struct APU *apu, uint8_t data)
{
    write_pulse_lo(&apu->pulse2, data);
}

void write_apu_square2_hi(struct APU *apu, uint8_t data)
{
    write_pulse_hi(&apu->pulse2, data);
}

void power_up_apu(struct APU *apu)
{
    struct APU a = {0};
    *apu = a;

    apu->pulse1.id = 1;
    apu->pulse2.id = 2;
}

void reset_apu(struct APU *apu)
{
    struct APU a = {0};
    *apu = a;

    apu->pulse1.id = 1;
    apu->pulse2.id = 2;
}

static float calculate_pulse_level(uint8_t value)
{
    static float output_table[32] = {0.f};
    static int table_built = 0;

    if (!table_built) {
        const int N = sizeof(output_table) / sizeof(output_table[0]);
        int i;
        for (i = 0; i < N; i++)
            output_table[i] = 95.88 / (8128. / i + 100);
        table_built = 1;
    }

    return output_table[value & 0x1F];
}

static uint8_t sample_pulse(struct pulse_channel *pulse)
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
        return pulse->envelope.volume;
    else
        return pulse->envelope.decay;
}

static void clock_pulse_timer(struct pulse_channel *pulse)
{
    if (pulse->timer == 0) {
        pulse->timer = pulse->timer_period;
        pulse->sequence_pos = (pulse->sequence_pos + 1) % 8;
    }
    else {
        pulse->timer--;
    }
}

static void clock_timers(struct APU *apu)
{
    clock_pulse_timer(&apu->pulse1);
    clock_pulse_timer(&apu->pulse2);
}

static void clock_length_counter(struct APU *apu)
{
    if (apu->pulse1.length > 0)
        apu->pulse1.length--;
    if (apu->pulse2.length > 0)
        apu->pulse2.length--;
}

static void calculate_target_period(struct pulse_channel *pulse)
{
    const uint16_t current_period = pulse->timer_period;
    struct sweep_unit *swp = &pulse->sweep;

    uint16_t change = current_period >> swp->shift;

    if (swp->negate) {
        swp->target_period = current_period - change;

        if (pulse->id == 1)
            swp->target_period--;
    }
    else {
        swp->target_period = current_period + change;
    }
}

static int is_sweep_muting(const struct pulse_channel *pulse)
{
    return pulse->timer_period < 8 || pulse->sweep.target_period > 0x07FF;
}

static void clock_sweep(struct pulse_channel *pulse)
{
    struct sweep_unit *swp = &pulse->sweep;

    if (swp->divider == 0 && swp->enabled && !is_sweep_muting(pulse)) {
        pulse->timer_period = swp->target_period;
        calculate_target_period(pulse);
    }

    if (swp->divider == 0 || swp->reload) {
        swp->divider = swp->period;
        swp->reload = 0;
    }
    else {
        swp->divider--;
    }
}

static void clock_envelope(struct envelope_unit *env)
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
    clock_sweep(&apu->pulse1);
    clock_sweep(&apu->pulse2);
}

static void clock_envelopes(struct APU *apu)
{
    clock_envelope(&apu->pulse1.envelope);
    clock_envelope(&apu->pulse2.envelope);
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
        clock_timers(apu);

    apu->audio_time += APU_TIME_STEP;

    if (apu->audio_time > AUDIO_SAMPLE_STEP) {
        /* generate a sample */
        apu->audio_time -= AUDIO_SAMPLE_STEP;

        const uint8_t p1 = sample_pulse(&apu->pulse1);
        const uint8_t p2 = sample_pulse(&apu->pulse2);
        const float pulse_out = calculate_pulse_level(p1 + p2);

        push_sample(0xFFFF * pulse_out);
    }
}
