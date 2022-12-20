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

    FrameBuffer fbuf;
    FrameBuffer patt;
    PPU ppu = {fbuf};
    CPU cpu = {ppu};

    void PowerUp();
    void ShutDown();

    void InsertCartridge(Cartridge *cart);
    void PushResetButton();
    void PlayGame();

    void UpdateFrame();
    void InputController(uint8_t id, uint8_t input);

    const Cartridge *GetCartridge() const { return cart_; }

    void Play();
    void Pause();
    bool IsPlaying() const { return is_playing_; }

private:
    uint64_t clock_ = 0;
    uint64_t frame_ = 0;

    Cartridge *cart_ = nullptr;

    // dma
    int dma_wait_ = 0;
    uint8_t dma_addr_ = 0;
    uint8_t dma_page_ = 0;
    uint8_t dma_data_ = 0;

    bool is_playing_ = false;

    void clock_dma();
};

} // namespace

#endif // _H
