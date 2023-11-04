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

enum BreakAt {
    NOWHERE = 0,
    NEXT_INSTRUCTION,
    NEXT_SCANLINE1,
    NEXT_SCANLINE8,
    NEXT_FRAME,
};

class NES {
public:
    NES() {}
    ~NES() {}

    FrameBuffer fbuf;
    FrameBuffer patt;
    FrameBuffer oam;
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
    void Pause();
    bool IsRunning() const;
    void StepTo(BreakAt stepto);

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
    bool is_running_ = true;
    BreakAt stepto_ = NOWHERE;
    int stepto_scanline_ = 0;

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

    void update_audio_speed();
    bool need_log() const;
    void print_disassemble() const;
};

} // namespace

#endif // _H
