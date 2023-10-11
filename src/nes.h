#ifndef NES_H
#define NES_H

#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "dma.h"
#include "disassemble.h"
#include "framebuffer.h"
#include "cartridge.h"
#include "serialize.h"
#include <cstdint>

namespace nes {

class Cartridge;
class FrameBuffer;

class NES {
public:
    NES() {}
    ~NES() {}

    FrameBuffer fbuf;
    FrameBuffer patt;
    PPU ppu = {fbuf};
    APU apu = {};
    CPU cpu = {ppu, apu};
    DMA dma = {cpu, ppu};

    void PowerUp();
    void ShutDown();

    void InsertCartridge(Cartridge *cart);
    void PushResetButton();
    void PlayGame();
    void StartLog();

    void UpdateFrame();
    void InputController(uint8_t id, uint8_t input);

    const Cartridge *GetCartridge() const { return cart_; }

    void Run();
    void Stop();
    bool IsRunning() const;
    void Step();

    // debug
    uint64_t GetLogLineCount() const;
    void SetChannelEnable(uint64_t chan_bits);
    uint64_t GetChannelEnable() const;

private:
    Cartridge *cart_ = nullptr;
    uint64_t frame_ = 0;
    bool do_log_ = false;
    uint64_t log_line_count_ = 0;

    // state
    enum EmulatorState {
        Running = 0,
        Stopped,
        Stepping,
    } state_ = Running;

    // serialization
    friend void Serialize(Archive &ar, const std::string &name, NES *data)
    {
        SERIALIZE_NAMESPACE_BEGIN(ar, name);
        SERIALIZE(ar, data, cpu);
        SERIALIZE(ar, data, ppu);
        SERIALIZE(ar, data, apu);
        SERIALIZE(ar, data, dma);
        SERIALIZE(ar, data, frame_);
        Serialize(ar, "cart_", data->cart_);
        SERIALIZE_NAMESPACE_END(ar);
    }

    bool need_log() const;
    void print_disassemble() const;
};

} // namespace

#endif // _H
