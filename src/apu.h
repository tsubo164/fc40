#ifndef APU_H
#define APU_H

#include <stdint.h>

namespace nes {

struct Sweep {
    uint8_t enabled = 0;
    uint8_t period = 0;
    uint16_t target_period = 0;
    uint8_t negate = 0;
    uint8_t shift = 0;
    uint8_t reload = 0;
    uint8_t divider = 0;
};

struct Envelope {
    uint8_t start = 0;
    uint8_t decay = 0;
    uint8_t divider = 0;
    uint8_t volume = 0;
    uint8_t loop = 0;
    uint8_t constant = 0;
};

struct PulseChannel {
    PulseChannel(uint8_t chan_id) : id(chan_id) {}

    uint8_t id = 0;
    uint8_t enabled = 0;
    uint8_t length = 0;
    uint8_t length_halt = 0;

    uint16_t timer = 0;
    uint16_t timer_period = 0;

    uint8_t duty = 0;
    uint8_t sequence_pos = 0;

    Sweep sweep;
    Envelope envelope;
};

struct TriangleChannel {
    uint8_t enabled = 0;
    uint8_t length = 0;
    uint8_t length_halt = 0;

    uint8_t control = 0;
    uint8_t linear_counter = 0;
    uint8_t linear_period = 0;
    uint8_t linear_reload = 0;

    uint16_t timer = 0;
    uint16_t timer_period = 0;

    uint8_t sequence_pos = 0;
};

struct NoiseChannel {
    uint8_t enabled = 0;
    uint8_t length = 0;
    uint8_t length_halt = 0;

    uint16_t shift = 1;
    uint8_t mode = 0;

    uint16_t timer = 0;
    uint16_t timer_period = 0;

    Envelope envelope;
};

struct APU {
    double audio_time;
    uint32_t clock;
    uint32_t cycle;

    uint8_t mode;
    uint8_t frame_interrupt;
    bool irq_generated = false;

    PulseChannel pulse1 = {1}, pulse2 = {2};
    TriangleChannel triangle;
    NoiseChannel noise;

    void Clock();
    void PowerUp();
    void Reset();

    void ClearIRQ();
    bool IsSetIRQ() const;
};

/* write registers */
extern void write_apu_status(struct APU *apu, uint8_t data);
extern void write_apu_frame_counter(struct APU *apu, uint8_t data);

extern void write_apu_square1_volume(struct APU *apu, uint8_t data);
extern void write_apu_square1_sweep(struct APU *apu, uint8_t data);
extern void write_apu_square1_lo(struct APU *apu, uint8_t data);
extern void write_apu_square1_hi(struct APU *apu, uint8_t data);

extern void write_apu_square2_volume(struct APU *apu, uint8_t data);
extern void write_apu_square2_sweep(struct APU *apu, uint8_t data);
extern void write_apu_square2_lo(struct APU *apu, uint8_t data);
extern void write_apu_square2_hi(struct APU *apu, uint8_t data);

extern void write_apu_triangle_linear(struct APU *apu, uint8_t data);
extern void write_apu_triangle_lo(struct APU *apu, uint8_t data);
extern void write_apu_triangle_hi(struct APU *apu, uint8_t data);

extern void write_apu_noise_volume(struct APU *apu, uint8_t data);
extern void write_apu_noise_lo(struct APU *apu, uint8_t data);
extern void write_apu_noise_hi(struct APU *apu, uint8_t data);

// read registers

} // namespace

#endif // _H
