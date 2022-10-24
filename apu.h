#ifndef APU_H
#define APU_H

#include <stdint.h>

struct sweep_unit {
    uint8_t enabled;
    uint8_t period;
    uint16_t target_period;
    uint8_t negate;
    uint8_t shift;
    uint8_t reload;
    uint8_t divider;
};

struct envelope_unit {
    uint8_t start;
    uint8_t decay;
    uint8_t divider;
    uint8_t volume;
    uint8_t loop;
    uint8_t constant;
};

struct pulse_channel {
    uint8_t id;
    uint8_t enabled;
    uint8_t length;
    uint8_t length_halt;

    uint16_t timer;
    uint16_t timer_period;

    uint8_t duty;
    uint8_t sequence_pos;

    struct sweep_unit sweep;
    struct envelope_unit envelope;
};

struct triangle_channel {
    uint8_t enabled;
    uint8_t length;
    uint8_t length_halt;

    uint8_t control;
    uint8_t linear_counter;
    uint8_t linear_period;
    uint8_t linear_reload;

    uint16_t timer;
    uint16_t timer_period;

    uint8_t sequence_pos;
};

struct noise_channel {
    uint8_t enabled;
    uint8_t length;
    uint8_t length_halt;

    uint16_t shift;

    uint8_t mode;
    uint16_t timer;
    uint16_t timer_period;

    struct envelope_unit envelope;
};

struct APU {
    double audio_time;
    uint32_t clock;
    uint32_t cycle;

    uint8_t mode;
    uint8_t interrupt;

    struct pulse_channel pulse1, pulse2;
    struct triangle_channel triangle;
    struct noise_channel noise;
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

/* read registers */

extern void power_up_apu(struct APU *apu);
extern void reset_apu(struct APU *apu);
extern void clock_apu(struct APU *apu);

#endif /* _H */
