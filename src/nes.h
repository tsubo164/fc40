#ifndef NES_H
#define NES_H

#include <cstdint>
#include "cpu.h"
#include "ppu.h"
#include "framebuffer.h"

namespace nes {

class Cartridge;
class FrameBuffer;

class NES {
public:
    NES() {}
    ~NES() {}

    struct CPU cpu = {0};
    PPU ppu;

    FrameBuffer fbuf;
    FrameBuffer patt;

    void PowerUp();
    void ShutDown();

    void InsertCartridge(Cartridge *cart);
    void PushResetButton();
    void PlayGame();

    void UpdateFrame();
    void InputController(uint8_t id, uint8_t input);

private:
    uint64_t clock_ = 0;
    uint64_t frame_ = 0;

    Cartridge *cart_ = nullptr;

    // dma
    int dma_wait_ = 0;
    uint8_t dma_addr_ = 0;
    uint8_t dma_page_ = 0;
    uint8_t dma_data_ = 0;

    void clock_dma();
};

} // namespace

#endif // _H
