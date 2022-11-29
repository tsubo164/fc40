#include "nes.h"
#include "framebuffer.h"
#include "display.h"
#include "sound.h"
#include "debug.h"

namespace nes {

constexpr int AUDIO_DELAY_FRAME = 2;

static void send_initial_samples();

void NES::PowerUp()
{
    const int RESX = 256;
    const int RESY = 240;

    // DMA
    dma_wait_ = 1;
    dma_addr_ = 0x00;
    dma_page_ = 0x00;
    dma_data_ = 0x00;

    // framebuffer
    fbuf.Resize(RESX, RESY);

    // pattern table
    patt.Resize(16 * 8 * 2, 16 * 8);
    LoadPatternTable(patt, cart_);

    // CPU and PPU
    cpu.PowerUp();
    ppu.PowerUp();
}

void NES::ShutDown()
{
}

void NES::InsertCartridge(Cartridge *cart)
{
    cart_ = cart;
    cpu.SetCartride(cart_);
    ppu.SetCartride(cart_);
}

void NES::PushResetButton()
{
    cpu.Reset();
    ppu.Reset();
}

void NES::PlayGame()
{
    Display disp(*this);

    InitSound();
    send_initial_samples();

    is_playing_ = true;

    disp.Open();
    FinishSound();

    is_playing_ = false;
}

void NES::UpdateFrame()
{
    if (frame_ % AUDIO_DELAY_FRAME == 0)
        PlaySamples();

    do {
        ppu.Clock();

        if (clock_ % 3 == 0) {
            if (cpu.IsSuspended())
                clock_dma();
            else
                cpu.Clock();

            cpu.ClockAPU();
        }

        if (ppu.IsSetNMI()) {
            ppu.ClearNMI();
            cpu.HandleNMI();
        }

        if (cpu.IsSetIRQ()) {
            cpu.ClearIRQ();
            cpu.HandleIRQ();
        }

        clock_++;

    } while (!ppu.IsFrameReady());

    if (frame_ % AUDIO_DELAY_FRAME == 0)
        SendSamples();

    frame_++;
}

void NES::InputController(uint8_t id, uint8_t input)
{
    cpu.InputController(id, input);
}

void NES::Play()
{
    PlaySamples();
    is_playing_ = true;
}

void NES::Pause()
{
    PauseSamples();
    is_playing_ = false;
}

void NES::clock_dma()
{
    if (dma_wait_) {
        if (clock_ % 2 == 1) {
            dma_wait_ = 0;
            dma_addr_ = 0x00;
            dma_page_ = cpu.GetDmaPage();
        }
        // idle for this cpu cycle
        return;
    }

    if (clock_ % 2 == 0) {
        // read
        dma_data_ = cpu.PeekData((dma_page_ << 8) | dma_addr_);
    }
    else {
        // write
        ppu.WriteDmaSprite(dma_addr_, dma_data_);
        dma_addr_++;

        if (dma_addr_ == 0x00) {
            dma_wait_ = 1;
            dma_page_ = 0x00;
            dma_addr_ = 0x00;
            cpu.Resume();
        }
    }
}

static void send_initial_samples()
{
    const int N = AUDIO_DELAY_FRAME * 44100 / 60;
    for (int i = 0; i < N; i++)
        PushSample(0.);
    SendSamples();
}

} // namespace
