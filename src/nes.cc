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

    Disassemble(assem_, *cart);
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
    if (!IsPlaying()) {
        if (need_disassemble_) {
            AssemblyCode assem;
            Disassemble2(assem, *cart_);

            auto found = assem.addr_map_.find(cpu.GetPC());
            printf("==============================\n");

            if (found != assem.addr_map_.end()) {
                const int index = found->second;
                for (int i = index - 16; i < index + 16; i++) {
                    if (i == index)
                        printf(" -> ");
                    else
                        printf("    ");
                    PrintLine(assem.instructions_[i]);
                }
            }
            else {
                printf("NOT FOUND PC!!! %04X\n", cpu.GetPC());
            }

            need_disassemble_ = false;
        }

        if (!is_stepping_)
            return;
    }

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

            if (is_stepping_ && cpu.GetCycles() == 0) {
                is_stepping_ = false;
                need_disassemble_ = true;
                return;
            }
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
    is_stepping_ = false;
}

void NES::Pause()
{
    PauseSamples();
    is_playing_ = false;
    need_disassemble_ = true;
}

bool NES::IsPlaying() const
{
    return is_playing_;
}

void NES::Step()
{
    if (!IsPlaying())
        is_stepping_ = true;
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
