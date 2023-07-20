#ifndef NES_H
#define NES_H

#include "cpu.h"
#include "ppu.h"
#include "dma.h"
#include "disassemble.h"
#include "framebuffer.h"
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
    CPU cpu = {ppu};
    DMA dma = {cpu, ppu};

    void PowerUp();
    void ShutDown();

    void InsertCartridge(Cartridge *cart);
    void PushResetButton();
    void PlayGame();

    void UpdateFrame();
    void UpdateFrame2();
    void InputController(uint8_t id, uint8_t input);

    const Cartridge *GetCartridge() const { return cart_; }

    void Run();
    void Stop();
    bool IsRunning() const;
    void Step();

private:
    uint64_t clock_ = 0;
    uint64_t frame_ = 0;

    Cartridge *cart_ = nullptr;

    // dma
    int dma_wait_ = 0;
    uint8_t dma_addr_ = 0;
    uint8_t dma_page_ = 0;
    uint8_t dma_data_ = 0;

    // state
    enum EmulatorState {
        Running = 0,
        Stopped,
        Stepping,
    } state_ = Running;

    void clock_dma();
    void print_disassemble() const;
};

} // namespace

#endif // _H
