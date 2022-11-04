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
    double audio_time = 0.;
    uint32_t clock = 0;
    uint32_t cycle = 0;

    uint8_t mode = 0;
    uint8_t frame_interrupt;
    bool irq_generated = false;

    PulseChannel pulse1 = {1}, pulse2 = {2};
    TriangleChannel triangle;
    NoiseChannel noise;

    // status
    void Clock();
    void PowerUp();
    void Reset();

    // interrupts
    void ClearIRQ();
    bool IsSetIRQ() const;

    // write registers
    void WriteStatus(uint8_t data);
    void WriteFrameCounter(uint8_t data);

    void WriteSquare1Volume(uint8_t data);
    void WriteSquare1Sweep(uint8_t data);
    void WriteSquare1Lo(uint8_t data);
    void WriteSquare1Hi(uint8_t data);

    void WriteSquare2Volume(uint8_t data);
    void WriteSquare2Sweep(uint8_t data);
    void WriteSquare2Lo(uint8_t data);
    void WriteSquare2Hi(uint8_t data);

    void WriteTriangleLinear(uint8_t data);
    void WriteTriangleLo(uint8_t data);
    void WriteTriangleHi(uint8_t data);
};

extern void write_apu_noise_volume(struct APU *apu, uint8_t data);
extern void write_apu_noise_lo(struct APU *apu, uint8_t data);
extern void write_apu_noise_hi(struct APU *apu, uint8_t data);

// read registers

} // namespace

#endif // _H
