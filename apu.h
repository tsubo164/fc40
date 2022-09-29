#ifndef APU_H
#define APU_H

#include <stdint.h>

struct APU {
    double audio_time;

    uint8_t pulse1_sweep_enabled;
    uint8_t pulse1_sweep_period;
    uint8_t pulse1_sweep_down;
    uint8_t pulse1_sweep_shift;
    uint8_t pulse1_sweep_reload;

    uint32_t pulse1_seq_sequence;
    uint32_t pulse1_seq_new_sequence;
    uint16_t pulse1_seq_reload;
    uint16_t pulse1_seq_timer;

    uint8_t pulse1_length_counter;
    uint8_t pulse1_halt;

    uint8_t pulse1_env_start;
    uint8_t pulse1_env_disable;
    uint16_t pulse1_env_volume;

    float pulse1_osc_dutycycle;
};

/* write registers */
extern void write_apu_square1_volume(struct APU *apu, uint8_t data);
extern void write_apu_square1_sweep(struct APU *apu, uint8_t data);
extern void write_apu_square1_lo(struct APU *apu, uint8_t data);
extern void write_apu_square1_hi(struct APU *apu, uint8_t data);

/* read registers */

extern void reset_apu(struct APU *apu);
extern void clock_apu(struct APU *apu);

#endif /* _H */
