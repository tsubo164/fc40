#include <stdlib.h>
#include <stdio.h>
#include "apu.h"
#include "sound.h"

namespace nes {

static const uint8_t length_table[] = {
     10, 254,  20,   2,  40,   4,  80,   6,
    160,   8,  60,  10,  14,  12,  26,  14,
     12,  16,  24,  18,  48,  20,  96,  22,
    192,  24,  72,  26,  16,  28,  32,  30
};

static void calculate_target_period(PulseChannel &pulse);

static void write_pulse_volume(PulseChannel &pulse, uint8_t data)
{
    /* $4000    | ddLC.VVVV | Pulse channel 1 duty and volume/envelope (write)
     * $4004    | ddLC.VVVV | Pulse channel 2 duty and volume/envelope (write)
     * bit 5    | --L- ---- | APU Length Counter halt flag/envelope loop flag
     * bit 4    | ---C ---- | Constant volume flag (0: use volume from envelope;
     *                                              1: use constant volume)
     * bits 3-0 | ---- VVVV | Used as the volume in constant volume (C set) mode.
     *                      | Also used as the reload value for the envelope's divider
     *                      | (the period becomes V + 1 quarter frames). */
    pulse.duty = (data >> 6) & 0x03;
    pulse.length_halt = (data >> 5) & 0x01;
    pulse.envelope.loop = pulse.length_halt;
    pulse.envelope.constant = (data >> 4) & 0x01;
    pulse.envelope.volume = (data & 0x0F);
}

static void write_pulse_sweep(PulseChannel &pulse, uint8_t data)
{
    /* $4001/$4005 | EPPP.NSSS | Pulse channel 1/2 sweep setup (write)
     * bit 7    | E--- ---- | Enabled flag
     * bits 6-4 | -PPP ---- | The divider's period is P + 1 half-frames
     * bit 3    | ---- N--- | Negate flag
     *          |           | 0: add to period, sweeping toward lower frequencies
     *          |           | 1: subtract from period, sweeping toward higher frequencies
     * bits 2-0 | ---- -SSS | Shift count (number of bits)
     * Side effects | Sets the reload flag */
    pulse.sweep.enabled = (data >> 7) & 0x01;
    /* The divider's period is set to P + 1 */
    pulse.sweep.period = ((data >> 4) & 0x07) + 1;
    pulse.sweep.negate = (data >> 3) & 0x01;
    pulse.sweep.shift = (data & 0x07);
    pulse.sweep.reload = 1;
}

static void write_pulse_lo(PulseChannel &pulse, uint8_t data)
{
    /* $4002/$4006 | LLLL.LLLL | Pulse 1/2 timer Low 8 bits */
    pulse.timer_period = (pulse.timer_period & 0xFF00) | data;
    calculate_target_period(pulse);
}

static void write_pulse_hi(PulseChannel &pulse, uint8_t data)
{
    /* $4003/$4007 | llll.lHHH | Pulse 1/2 length counter load and timer High 3 bits
     *
     * The sequencer is immediately restarted at the first value of the current sequence.
     * The envelope is also restarted. The period divider is not reset.  */
    pulse.length = length_table[data >> 3];
    pulse.timer_period = ((data & 0x07) << 8) | (pulse.timer_period & 0x00FF);
    pulse.sequence_pos = 0;
    pulse.envelope.start = 1;

    /* Whenever the current period changes for any reason, whether by $400x writes or
     * by sweep, the target period also changes. */
    calculate_target_period(pulse);
}

static void write_triangle_linear(TriangleChannel &tri, uint8_t data)
{
    /* $4008    | CRRR.RRRR | Linear counter setup (write)
     * bit 7    | C---.---- | Control flag (this bit is also the length counter halt flag)
     * bits 6-0 | -RRR RRRR | Counter reload value */
    tri.length_halt = (data >> 7) & 0x01;
    tri.control = tri.length_halt;
    tri.linear_period = (data & 0x7F);
}

static void write_triangle_lo(TriangleChannel &tri, uint8_t data)
{
    /* $400A    | LLLL.LLLL | Timer low (write)
     * bits 7-0 | LLLL LLLL | Timer low 8 bits */
    tri.timer_period = (tri.timer_period & 0xFF00) | data;
}

static void write_triangle_hi(TriangleChannel &tri, uint8_t data)
{
    /* $400B     | llll.lHHH | Length counter load and timer high (write)
     * bits 2-0  | ---- -HHH | Timer high 3 bits
     * Side effects | Sets the linear counter reload flag */
    tri.length = length_table[data >> 3];
    tri.timer_period = ((data & 0x07) << 8) | (tri.timer_period & 0x00FF);
    tri.linear_reload = 1;
}

static void write_noise_volume(NoiseChannel &noise, uint8_t data)
{
    /* $400C    | --LC.VVVV | Noise channel volume/envelope (write)
     * bit 5    | --L- ---- | APU Length Counter halt flag/envelope loop flag
     * bit 4    | ---C ---- | Constant volume flag (0: use volume from envelope;
     *                                              1: use constant volume)
     * bits 3-0 | ---- VVVV | Used as the volume in constant volume (C set) mode.
     *                      | Also used as the reload value for the envelope's divider
     *                      | (the period becomes V + 1 quarter frames). */
    noise.length_halt = (data >> 5) & 0x01;
    noise.envelope.loop = noise.length_halt;
    noise.envelope.constant = (data >> 4) & 0x01;
    noise.envelope.volume = (data & 0x0F);
}

static void write_noise_lo(NoiseChannel &noise, uint8_t data)
{
    /* $400E    | M---.PPPP | Mode and period (write)
     * bit 7    | M--- ---- | Mode flag
     * bits 3-0 | ---- PPPP | The timer period is set to entry P of the following:
     * Rate  $0 $1  $2  $3  $4  $5   $6   $7   $8   $9   $A   $B   $C    $D    $E    $F
     * --------------------------------------------------------------------------
     * NTSC   4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
     * PAL    4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708,  944, 1890, 3778 */
    static const uint16_t period_table[] = {
        4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
    };
    noise.mode = (data >> 7) & 0x01;
    noise.timer_period = period_table[data & 0x0F];
}

static void write_noise_hi(NoiseChannel &noise, uint8_t data)
{
    /* $400F | llll.l--- | Length counter load and envelope restart (write) */
    noise.length = length_table[data >> 3];
    noise.envelope.start = 1;
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

    apu->triangle.enabled = (data >> 2) & 0x01;
    if (!apu->triangle.enabled)
        apu->triangle.length = 0;

    apu->noise.enabled = (data >> 3) & 0x01;
    if (!apu->noise.enabled)
        apu->noise.length = 0;
}

void write_apu_frame_counter(struct APU *apu, uint8_t data)
{
    /* $4017 | MI--.---- | Set mode and interrupt (write)
     * Bit 7 | M--- ---- | Sequencer mode: 0 selects 4-step sequence,
     *       |           | 1 selects 5-step sequence
     * Bit 6 | -I-- ---- | Interrupt inhibit flag. If set, the frame interrupt flag
     *       |           | is cleared, otherwise it is unaffected.
     * Side effects | After 3 or 4 CPU clock cycles*, the timer is reset.
     *              | If the mode flag is set, then both "quarter frame" and
     *              | "half frame" signals are also generated. */
    apu->mode = (data >> 7) & 0x01;
    apu->frame_interrupt = ((data >> 6) & 0x01) == 0;
}

void write_apu_square1_volume(struct APU *apu, uint8_t data)
{
    write_pulse_volume(apu->pulse1, data);
}

void write_apu_square1_sweep(struct APU *apu, uint8_t data)
{
    write_pulse_sweep(apu->pulse1, data);
}

void write_apu_square1_lo(struct APU *apu, uint8_t data)
{
    write_pulse_lo(apu->pulse1, data);
}

void write_apu_square1_hi(struct APU *apu, uint8_t data)
{
    write_pulse_hi(apu->pulse1, data);
}

void write_apu_square2_volume(struct APU *apu, uint8_t data)
{
    write_pulse_volume(apu->pulse2, data);
}

void write_apu_square2_sweep(struct APU *apu, uint8_t data)
{
    write_pulse_sweep(apu->pulse2, data);
}

void write_apu_square2_lo(struct APU *apu, uint8_t data)
{
    write_pulse_lo(apu->pulse2, data);
}

void write_apu_square2_hi(struct APU *apu, uint8_t data)
{
    write_pulse_hi(apu->pulse2, data);
}

void write_apu_triangle_linear(struct APU *apu, uint8_t data)
{
    write_triangle_linear(apu->triangle, data);
}

void write_apu_triangle_lo(struct APU *apu, uint8_t data)
{
    write_triangle_lo(apu->triangle, data);
}

void write_apu_triangle_hi(struct APU *apu, uint8_t data)
{
    write_triangle_hi(apu->triangle, data);
}

void write_apu_noise_volume(struct APU *apu, uint8_t data)
{
    write_noise_volume(apu->noise, data);
}

void write_apu_noise_lo(struct APU *apu, uint8_t data)
{
    write_noise_lo(apu->noise, data);
}

void write_apu_noise_hi(struct APU *apu, uint8_t data)
{
    write_noise_hi(apu->noise, data);
}

static float calculate_pulse_level(uint8_t pulse1, uint8_t pulse2)
{
    /* Linear Approximation sounds less cracking/popping */
    return 0.00752 * (pulse1 + pulse2);

    static float output_table[32] = {0.f};
    static int table_built = 0;

    if (!table_built) {
        const int N = sizeof(output_table) / sizeof(output_table[0]);
        int i;
        for (i = 0; i < N; i++)
            output_table[i] = 95.88 / (8128. / i + 100);
        table_built = 1;
    }

    return output_table[(pulse1 + pulse2) & 0x1F];
}

static float calculate_tnd_level(uint8_t triangle, uint8_t noise, uint8_t dmc)
{
    /* Linear Approximation sounds less cracking/popping */
    return 0.00851 * triangle + 0.00494 * noise + 0.00335 * dmc;

    if (triangle == 0 && noise == 0 && dmc == 0)
        return 0.;

    const float tnd = triangle / 8227. + noise / 12241. + dmc / 22638.;
    return 159.79 / (1. / tnd + 100.);
}

static uint8_t sample_pulse(PulseChannel &pulse)
{
    const static uint8_t sequence_table[][8] = {
        {0, 1, 0, 0, 0, 0, 0, 0}, /* (12.5%) */
        {0, 1, 1, 0, 0, 0, 0, 0}, /* (25%) */
        {0, 1, 1, 1, 1, 0, 0, 0}, /* (50%) */
        {1, 0, 0, 1, 1, 1, 1, 1}  /* (25% negated) */
    };

    const uint8_t sample = sequence_table[pulse.duty][pulse.sequence_pos];

    if (sample == 0)
        return 0;

    if (pulse.enabled == 0)
        return 0;

    if (pulse.length == 0)
        return 0;

    if (pulse.timer_period < 8 || pulse.timer_period > 0x7FF)
        return 0;

    if (pulse.envelope.constant)
        return pulse.envelope.volume;
    else
        return pulse.envelope.decay;
}

static uint8_t sample_triangle(TriangleChannel &tri)
{
    const static uint8_t sequence_table[32] = {
        15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
    };

    const uint8_t sample = sequence_table[tri.sequence_pos];

    if (sample == 0)
        return 0;

    if (tri.enabled == 0)
        return 0;

    if (tri.length == 0)
        return 0;

    if (tri.timer_period < 2)
        return 0;

    if (tri.linear_counter == 0)
        return 0;

    return sample;
}

static uint8_t sample_noise(NoiseChannel &noise)
{
    if (noise.enabled == 0)
        return 0;

    if (noise.length == 0)
        return 0;

    if ((noise.shift & 0x0001) == 1)
        return 0;

    if (noise.envelope.constant)
        return noise.envelope.volume;
    else
        return noise.envelope.decay;
}

static void clock_pulse_timer(PulseChannel &pulse)
{
    if (pulse.timer == 0) {
        pulse.timer = pulse.timer_period;
        pulse.sequence_pos = (pulse.sequence_pos + 1) % 8;
    }
    else {
        pulse.timer--;
    }
}

static void clock_triangle_timer(TriangleChannel &tri)
{
    if (tri.timer == 0) {
        tri.timer = tri.timer_period;
        if (tri.length > 0 && tri.linear_counter > 0)
            tri.sequence_pos = (tri.sequence_pos + 1) % 32;
    }
    else {
        tri.timer--;
    }
}

static void clock_noise_timer(NoiseChannel &noise)
{
    /* The shift register is 15 bits wide, with bits numbered
     * 14 - 13 - 12 - 11 - 10 - 9 - 8 - 7 - 6 - 5 - 4 - 3 - 2 - 1 - 0
     * When the timer clocks the shift register, the following actions occur in order:
     * 1. Feedback is calculated as the exclusive-OR of bit 0 and one other bit:
     *    bit 6 if Mode flag is set, otherwise bit 1.
     * 2. The shift register is shifted right by one bit.
     * 3. Bit 14, the leftmost bit, is set to the feedback calculated earlier. */
    if (noise.timer == 0) {
        noise.timer = noise.timer_period;

        const uint8_t bit = noise.mode == 1 ? 6 : 1;
        const uint16_t other = (noise.shift >> bit) & 0x01;
        const uint16_t feedback = (noise.shift & 0x01) ^ other;

        noise.shift >>= 1;
        noise.shift |= (feedback << 14);
    }
    else {
        noise.timer--;
    }
}

static void clock_timers(struct APU *apu)
{
    if (apu->clock % 2 == 0) {
        clock_pulse_timer(apu->pulse1);
        clock_pulse_timer(apu->pulse2);
        clock_noise_timer(apu->noise);
    }

    clock_triangle_timer(apu->triangle);
}

static void clock_length_counters(struct APU *apu)
{
    if (apu->pulse1.length > 0 && !apu->pulse1.length_halt)
        apu->pulse1.length--;

    if (apu->pulse2.length > 0 && !apu->pulse1.length_halt)
        apu->pulse2.length--;

    if (apu->triangle.length > 0 && !apu->triangle.length_halt)
        apu->triangle.length--;

    if (apu->noise.length > 0 && !apu->noise.length_halt)
        apu->noise.length--;
}

static void calculate_target_period(PulseChannel &pulse)
{
    const uint16_t current_period = pulse.timer_period;
    Sweep &swp = pulse.sweep;

    uint16_t change = current_period >> swp.shift;

    if (swp.negate) {
        swp.target_period = current_period - change;

        if (pulse.id == 1)
            swp.target_period--;
    }
    else {
        swp.target_period = current_period + change;
    }
}

static int is_sweep_muting(const PulseChannel &pulse)
{
    return pulse.timer_period < 8 || pulse.sweep.target_period > 0x07FF;
}

static void clock_sweep(PulseChannel &pulse)
{
    Sweep &swp = pulse.sweep;

    if (swp.divider == 0 && swp.enabled && !is_sweep_muting(pulse)) {
        pulse.timer_period = swp.target_period;
        calculate_target_period(pulse);
    }

    if (swp.divider == 0 || swp.reload) {
        swp.divider = swp.period;
        swp.reload = 0;
    }
    else {
        swp.divider--;
    }
}

static void clock_envelope(Envelope &env)
{
    if (env.start == 0) {
        /* clock divider */
        if (env.divider == 0) {
            env.divider = env.volume;

            if (env.decay > 0)
                env.decay--;
            else if (env.loop)
                env.decay = 15;
        }
        else {
            env.divider--;
        }
    }
    else {
        env.start = 0;
        env.decay = 15;
        env.divider = env.volume;
    }
}

static void clock_sweeps(struct APU *apu)
{
    clock_sweep(apu->pulse1);
    clock_sweep(apu->pulse2);
}

static void clock_envelopes(struct APU *apu)
{
    clock_envelope(apu->pulse1.envelope);
    clock_envelope(apu->pulse2.envelope);
    clock_envelope(apu->noise.envelope);
}

static void clock_linear_counter(struct APU *apu)
{
    TriangleChannel &tri = apu->triangle;

    if (tri.linear_reload) {
        tri.linear_counter = tri.linear_period;
    }
    else if (tri.linear_counter > 0) {
        tri.linear_counter--;
    }

    if (!tri.control)
        tri.linear_reload = 0;
}

static void clock_frame_interrupt(struct APU *apu)
{
    if (apu->frame_interrupt)
        apu->irq_generated = 1;
}

static void clock_sequencer_step4(struct APU *apu)
{
    switch (apu->cycle) {
    case 3729:
    case 11186:
        clock_envelopes(apu);
        clock_linear_counter(apu);
        break;

    case 7457:
    case 14915:
        clock_envelopes(apu);
        clock_linear_counter(apu);
        clock_length_counters(apu);
        clock_sweeps(apu);
        break;

    default:
        break;
    }

    if (apu->cycle == 14915) {
        clock_frame_interrupt(apu);
        apu->cycle = 0;
    } else {
        apu->cycle++;
    }
}

static void clock_sequencer_step5(struct APU *apu)
{
    switch (apu->cycle) {
    case 3729:
    case 11186:
        clock_envelopes(apu);
        clock_linear_counter(apu);
        break;

    case 7457:
    case 18641:
        clock_envelopes(apu);
        clock_linear_counter(apu);
        clock_length_counters(apu);
        clock_sweeps(apu);
        break;

    default:
        break;
    }

    if (apu->cycle == 18641)
        apu->cycle = 0;
    else
        apu->cycle++;
}

static void clock_sequencer(struct APU *apu)
{
    if (apu->mode == 0)
        clock_sequencer_step4(apu);
    else
        clock_sequencer_step5(apu);
}

constexpr int CPU_CLOCK_FREQ = 1789773;
constexpr double APU_TIME_STEP = 1. / CPU_CLOCK_FREQ;
constexpr double AUDIO_SAMPLE_STEP = 1. / 44100;

void APU::Clock()
{
    /* apu clocked every other cpu cycles */
    if (clock % 2 == 0)
        clock_sequencer(this);

    clock_timers(this);

    audio_time += APU_TIME_STEP;

    if (audio_time > AUDIO_SAMPLE_STEP) {
        /* generate a sample */
        audio_time -= AUDIO_SAMPLE_STEP;

        const uint8_t p1 = sample_pulse(pulse1);
        const uint8_t p2 = sample_pulse(pulse2);
        const float pulse_out = calculate_pulse_level(p1, p2);

        const uint8_t t = sample_triangle(triangle);
        const uint8_t n = sample_noise(noise);
        const float tnd_out = calculate_tnd_level(t, n, 0);

        static float lpf = 0;
        const float raw_data = pulse_out + tnd_out;
        const float k = .5;

        /* Lowpass Filter: lpf = (1 - k) * prev_lpf + k * raw_data */
        lpf += k * (raw_data - lpf);

        PushSample(lpf);
    }

    clock++;
}

void APU::PowerUp()
{
    Reset();
}

void APU::Reset()
{
    *this = APU();

    // On power-up, the shift register is loaded with the value 1.
    noise.shift = 1;
}

void APU::ClearIRQ()
{
    irq_generated = 0;
}

bool APU::IsSetIRQ() const
{
    return irq_generated;
}

} // namespace
