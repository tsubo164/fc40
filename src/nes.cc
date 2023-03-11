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

    state_ = Running;

    disp.Open();
    FinishSound();

    state_ = Stopped;
}

void NES::UpdateFrame()
{
    if (state_ == Stopped)
        return;

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

            if (state_ == Stepping && cpu.GetCycles() == 0) {
                state_ = Stopped;
                print_disassemble();
                clock_++;
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

void NES::Run()
{
    PlaySamples();
    state_ = Running;
}

void NES::Stop()
{
    PauseSamples();
    Step();
}

bool NES::IsRunning() const
{
    return state_ == Running;
}

void NES::Step()
{
    state_ = Stepping;
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

static void print_cpu_status(CpuStatus stat)
{
    const char indent[] = "    ";
    printf("%sN V U B D I Z C\n", indent);

    char flags[] = "- - - - - - - -\n";
    uint8_t mask = (0x01 << 7);

    for (int i = 0; i < 8; i++) {
        flags[i * 2] = (stat.p & mask) ? '1' : '-';
        mask >>= 1;
    }
    printf("%s%s", indent, flags);

    printf("%sA  X  Y  SP\n", indent);
    printf("%s%02X %02X %02X %02X\n", indent, stat.a, stat.x, stat.y, stat.s);
}

void NES::print_disassemble() const
{
    printf("==============================\n");
    print_cpu_status(cpu.GetStatus());
    printf("------------------------------\n");

    AssemblyCode assem;
    Disassemble(assem, *cart_);

    const int index = assem.FindCode(cpu.GetPC());
    if (index != -1) {
        const int start = std::max(index - 16, 0);
        const int end   = std::min(index + 16, assem.GetCount());

        for (int i = start; i < end; i++) {
            const Code code = assem.GetCode(i);
            const std::string line = GetCodeString(code);

            if (i == index) {
                printf(" -> ");
                printf("%s", line.c_str());

                const std::string mem = GetMemoryString(code, cpu);
                printf("%s", mem.c_str());
            } else {
                printf("    ");
                printf("%s", line.c_str());
            }

            printf("\n");
        }
    }
    else {
        printf("NOT FOUND PC!!! %04X\n", cpu.GetPC());
    }
}

} // namespace
