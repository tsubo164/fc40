#ifndef APU_H
#define APU_H

#include "cpu.h"
#include <cstdint>

namespace nes {

struct Sweep {
    bool enabled = false;
    uint8_t period = 0;
    uint16_t target_period = 0;
    bool negate = false;
    uint8_t shift = 0;
    bool reload = false;
    uint8_t divider = 0;

    // serialization
    friend void Serialize(Archive &ar, const std::string &name, Sweep *data)
    {
        SERIALIZE_NAMESPACE_BEGIN(ar, name);
        SERIALIZE(ar, data, enabled);
        SERIALIZE(ar, data, period);
        SERIALIZE(ar, data, target_period);
        SERIALIZE(ar, data, negate);
        SERIALIZE(ar, data, shift);
        SERIALIZE(ar, data, reload);
        SERIALIZE(ar, data, divider);
        SERIALIZE_NAMESPACE_END(ar);
    }
};

struct Envelope {
    bool start = false;
    uint8_t decay = 0;
    uint8_t divider = 0;
    uint8_t volume = 0;
    bool loop = false;
    bool constant = false;

    // serialization
    friend void Serialize(Archive &ar, const std::string &name, Envelope *data)
    {
        SERIALIZE_NAMESPACE_BEGIN(ar, name);
        SERIALIZE(ar, data, start);
        SERIALIZE(ar, data, decay);
        SERIALIZE(ar, data, divider);
        SERIALIZE(ar, data, volume);
        SERIALIZE(ar, data, loop);
        SERIALIZE(ar, data, constant);
        SERIALIZE_NAMESPACE_END(ar);
    }
};

struct PulseChannel {
    // timer
    bool enabled = false;
    uint16_t timer = 0;
    uint16_t timer_period = 0;

    // length
    uint8_t length = 0;
    bool length_halt = false;

    uint8_t duty = 0;
    uint8_t sequence_pos = 0;

    Sweep sweep;
    Envelope envelope;

    // serialization
    friend void Serialize(Archive &ar, const std::string &name, PulseChannel *data)
    {
        SERIALIZE_NAMESPACE_BEGIN(ar, name);
        SERIALIZE(ar, data, enabled);
        SERIALIZE(ar, data, timer);
        SERIALIZE(ar, data, timer_period);
        SERIALIZE(ar, data, length);
        SERIALIZE(ar, data, length_halt);
        SERIALIZE(ar, data, duty);
        SERIALIZE(ar, data, sequence_pos);
        SERIALIZE(ar, data, sweep);
        SERIALIZE(ar, data, envelope);
        SERIALIZE_NAMESPACE_END(ar);
    }
};

struct TriangleChannel {
    // timer
    bool enabled = false;
    uint16_t timer = 0;
    uint16_t timer_period = 0;

    // length
    uint8_t length = 0;
    bool length_halt = false;

    uint8_t control = 0;
    uint8_t linear_counter = 0;
    uint8_t linear_period = 0;
    uint8_t linear_reload = 0;

    uint8_t sequence_pos = 0;

    // output
    float output_level = 0;
    float start_ramp = 0;

    // serialization
    friend void Serialize(Archive &ar, const std::string &name, TriangleChannel *data)
    {
        SERIALIZE_NAMESPACE_BEGIN(ar, name);
        SERIALIZE(ar, data, enabled);
        SERIALIZE(ar, data, timer);
        SERIALIZE(ar, data, timer_period);
        SERIALIZE(ar, data, length);
        SERIALIZE(ar, data, length_halt);
        SERIALIZE(ar, data, control);
        SERIALIZE(ar, data, linear_counter);
        SERIALIZE(ar, data, linear_period);
        SERIALIZE(ar, data, linear_reload);
        SERIALIZE(ar, data, sequence_pos);
        SERIALIZE(ar, data, output_level);
        SERIALIZE(ar, data, start_ramp);
        SERIALIZE_NAMESPACE_END(ar);
    }
};

struct NoiseChannel {
    // timer
    bool enabled = false;
    uint16_t timer = 0;
    uint16_t timer_period = 0;

    // length
    uint8_t length = 0;
    bool length_halt = false;

    uint16_t shift = 1;
    uint8_t mode = 0;

    Envelope envelope;

    // serialization
    friend void Serialize(Archive &ar, const std::string &name, NoiseChannel *data)
    {
        SERIALIZE_NAMESPACE_BEGIN(ar, name);
        SERIALIZE(ar, data, enabled);
        SERIALIZE(ar, data, timer);
        SERIALIZE(ar, data, timer_period);
        SERIALIZE(ar, data, length);
        SERIALIZE(ar, data, length_halt);
        SERIALIZE(ar, data, shift);
        SERIALIZE(ar, data, mode);
        SERIALIZE(ar, data, envelope);
        SERIALIZE_NAMESPACE_END(ar);
    }
};

struct DmcChannel {
    // timer
    bool enabled = false;
    uint16_t timer = 0;
    uint16_t timer_period = 0;

    // flags
    bool irq_generated = false;
    bool irq_enabled = false;
    bool cpu_stall = false;

    // sample
    bool buffer_empty = true;
    uint8_t sample_buffer = 0;

    // memory reader
    bool loop = false;
    const CPU *cpu = nullptr;
    uint16_t sample_address = 0;
    uint16_t sample_length = 0;
    uint16_t current_address = 0;
    uint16_t bytes_remaining = 0;

    // output unit
    uint8_t output_level = 0;
    uint8_t shift_register = 0;
    uint8_t bits_counter = 0;

    // serialization
    friend void Serialize(Archive &ar, const std::string &name, DmcChannel *data)
    {
        SERIALIZE_NAMESPACE_BEGIN(ar, name);
        SERIALIZE(ar, data, enabled);
        SERIALIZE(ar, data, timer);
        SERIALIZE(ar, data, timer_period);
        SERIALIZE(ar, data, irq_generated);
        SERIALIZE(ar, data, irq_enabled);
        SERIALIZE(ar, data, cpu_stall);
        SERIALIZE(ar, data, buffer_empty);
        SERIALIZE(ar, data, sample_buffer);
        SERIALIZE(ar, data, loop);
        SERIALIZE(ar, data, sample_address);
        SERIALIZE(ar, data, sample_length);
        SERIALIZE(ar, data, current_address);
        SERIALIZE(ar, data, bytes_remaining);
        SERIALIZE(ar, data, output_level);
        SERIALIZE(ar, data, shift_register);
        SERIALIZE(ar, data, bits_counter);
        SERIALIZE_NAMESPACE_END(ar);
    }
};

class APU {
public:
    APU() {}
    ~APU() {}

    // status
    bool Run(int cpu_cycles);
    void Clock();
    void PowerUp();
    void Reset();

    // interrupts
    bool IsSetIRQ() const;
    // CPU
    void SetCPU(const CPU *cpu);
    void SetSpeedFactor(float factor);

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

    void WriteNoiseVolume(uint8_t data);
    void WriteNoiseLo(uint8_t data);
    void WriteNoiseHi(uint8_t data);

    void WriteDmcFrequency(uint8_t data);
    void WriteDmcLoadCounter(uint8_t data);
    void WriteDmcSampleAddress(uint8_t data);
    void WriteDmcSampleLength(uint8_t data);

    // read registers
    uint8_t ReadStatus();
    uint8_t PeekStatus() const;

    // debug
    void SetChannelEnable(uint8_t chan_bits);
    uint8_t GetChannelEnable() const;

private:
    float audio_time_ = 0.f;
    float speed_factor_ = 1.f;
    float new_factor_ = 1.f;
    bool  factor_changed_ = false;

    uint32_t clock_ = 0;
    uint32_t cycle_ = 0;

    uint8_t mode_ = 0;
    bool inhibit_interrupt_ = false;
    bool frame_interrupt_ = false;
    bool dmc_interrupt_ = false;
    float low_pass_filter_ = 0.f;

    PulseChannel pulse1_, pulse2_;
    TriangleChannel triangle_;
    NoiseChannel noise_;
    DmcChannel dmc_;

    // debug
    uint8_t chan_enable_ = 0x1F;

    // serialization
    friend void Serialize(Archive &ar, const std::string &name, APU *data)
    {
        SERIALIZE_NAMESPACE_BEGIN(ar, name);
        SERIALIZE(ar, data, audio_time_);
        SERIALIZE(ar, data, clock_);
        SERIALIZE(ar, data, cycle_);
        SERIALIZE(ar, data, mode_);
        SERIALIZE(ar, data, inhibit_interrupt_);
        SERIALIZE(ar, data, frame_interrupt_);
        SERIALIZE(ar, data, dmc_interrupt_);
        SERIALIZE(ar, data, low_pass_filter_);
        SERIALIZE(ar, data, pulse1_);
        SERIALIZE(ar, data, pulse2_);
        SERIALIZE(ar, data, triangle_);
        SERIALIZE(ar, data, noise_);
        SERIALIZE(ar, data, dmc_);
        SERIALIZE_NAMESPACE_END(ar);
    }

    void clock_timers();
    void clock_length_counters();
    void clock_sweeps();
    void clock_envelopes();
    void clock_linear_counter();
    void clock_frame_interrupt();
    void clock_sequencer_step4();
    void clock_sequencer_step5();
    void clock_sequencer();
};

} // namespace

#endif // _H
