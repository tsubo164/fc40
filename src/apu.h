#ifndef APU_H
#define APU_H

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
};

struct Envelope {
    bool start = false;
    uint8_t decay = 0;
    uint8_t divider = 0;
    uint8_t volume = 0;
    bool loop = false;
    bool constant = false;
};

struct PulseChannel {
    bool enabled = false;
    uint8_t length = 0;
    bool length_halt = false;

    uint16_t timer = 0;
    uint16_t timer_period = 0;

    uint8_t duty = 0;
    uint8_t sequence_pos = 0;

    Sweep sweep;
    Envelope envelope;
};

struct TriangleChannel {
    bool enabled = false;
    uint8_t length = 0;
    bool length_halt = false;

    uint8_t control = 0;
    uint8_t linear_counter = 0;
    uint8_t linear_period = 0;
    uint8_t linear_reload = 0;

    uint16_t timer = 0;
    uint16_t timer_period = 0;

    uint8_t sequence_pos = 0;
};

struct NoiseChannel {
    bool enabled = false;
    uint8_t length = 0;
    bool length_halt = false;

    uint16_t shift = 1;
    uint8_t mode = 0;

    uint16_t timer = 0;
    uint16_t timer_period = 0;

    Envelope envelope;
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
    bool loop = false;
    bool empty = true;
    bool silent = true;

    // shift register
    uint8_t shift = 0;
    uint8_t bits_remaining = 0;

    // samples
    uint8_t output_level = 0;
    uint16_t sample_address = 0;
    uint16_t sample_length = 0;
    uint16_t sample_buffer = 0;
    uint16_t bytes_remaining = 0;
    uint16_t byte_addr = 0;

    // sample
    struct Sample {
        bool empty = true;
        uint8_t buffer = 0;
    } sample;

    // memory reader
    struct MemoryReader {
        uint16_t start = 0;
        uint16_t length = 0;
        uint16_t addr = 0;
        uint16_t remaining = 0;
    } memory;

    // output unit
    struct OutputUnit {
        bool silent = true;
        uint8_t level = 0;
        uint8_t shift = 0;
        uint8_t bits = 0;
    } output;
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
    double audio_time_ = 0.;
    uint32_t clock_ = 0;
    uint32_t cycle_ = 0;

    uint8_t mode_ = 0;
    bool inhibit_interrupt_ = false;
    bool frame_interrupt_ = false;
    bool dmc_interrupt_ = false;

    PulseChannel pulse1_, pulse2_;
    TriangleChannel triangle_;
    NoiseChannel noise_;
    DmcChannel dmc_;
    uint8_t chan_enable_ = 0x1F;

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
