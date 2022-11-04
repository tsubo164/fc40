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

    // dma
    dma_wait_ = 1;
    dma_addr_ = 0x00;
    dma_page_ = 0x00;
    dma_data_ = 0x00;

    // framebuffer
    fbuf.Resize(RESX, RESY);
    ppu.fbuf = &fbuf;

    // pattern table
    patt.Resize(16 * 8 * 2, 16 * 8);
    LoadPatternTable(patt, cart_);

    // CPU and PPU
    cpu.ppu = &ppu;
    power_up_cpu(&cpu);
    ppu.PowerUp();
}

void NES::ShutDown()
{
}

void NES::InsertCartridge(Cartridge *cart)
{
    cart_ = cart;
    cpu.cart = cart_;
    ppu.cart = cart_;
}

void NES::PushResetButton()
{
    reset_cpu(&cpu);
    ppu.Reset();
}

void NES::PlayGame()
{
    Display disp(*this);

    InitSound();
    send_initial_samples();

    disp.Open();
    FinishSound();
}

void NES::UpdateFrame()
{
    if (frame_ % AUDIO_DELAY_FRAME == 0)
        PlaySamples();

    do {
        ppu.Clock();

        if (clock_ % 3 == 0) {
            if (is_suspended(&cpu))
                clock_dma();
            else
                clock_cpu(&cpu);

            cpu.apu.Clock();
        }

        if (ppu.IsSetNMI()) {
            ppu.ClearNMI();
            nmi(&cpu);
        }

        if (cpu.apu.IsSetIRQ()) {
            cpu.apu.ClearIRQ();
            irq(&cpu);
        }

        clock_++;

    } while (!ppu.IsFrameReady());

    if (frame_ % AUDIO_DELAY_FRAME == 0)
        SendSamples();

    frame_++;
}

void NES::InputController(uint8_t id, uint8_t input)
{
    set_controller_input(&cpu, id, input);
}

void NES::clock_dma()
{
    if (dma_wait_) {
        if (clock_ % 2 == 1) {
            dma_wait_ = 0;
            dma_addr_ = 0x00;
            dma_page_ = get_dma_page(&cpu);
        }
        // idle for this cpu cycle
        return;
    }

    if (clock_ % 2 == 0) {
        // read
        dma_data_ = read_cpu_data(&cpu, (dma_page_ << 8) | dma_addr_);
    }
    else {
        // write
        write_dma_sprite(&ppu, dma_addr_, dma_data_);
        dma_addr_++;

        if (dma_addr_ == 0x00) {
            dma_wait_ = 1;
            dma_page_ = 0x00;
            dma_addr_ = 0x00;
            resume(&cpu);
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
