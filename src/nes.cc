#include "nes.h"
#include "framebuffer.h"
#include "cartridge.h"
#include "display.h"
#include "sound.h"
#include "debug.h"

namespace nes {

static constexpr int AUDIO_DELAY_FRAME = 1;

static void send_initial_samples();

void NES::PowerUp()
{
    const int RESX = 256;
    const int RESY = 240;

    // framebuffer
    fbuf.Resize(RESX, RESY);

    // pattern table
    patt.Resize(16 * 8 * 2, 16 * 8);

    // OAM table
    oam.Resize(16 * 8, 4 * 8);

    // CPU and PPU
    cpu.PowerUp();
    ppu.PowerUp();
    apu.PowerUp();
    dma.PowerUp();

    apu.SetCPU(&cpu);
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
    apu.Reset();
    dma.Reset();
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

void NES::StartLog()
{
    do_log_ = true;
}

bool NES::need_log() const
{
    return do_log_ && !cpu.IsSuspended();
}

void NES::update_audio_speed()
{
    static constexpr int WARN_SPEED_CHANGE = 0;

    const int count = GetQueuedSampleCount();
    const float ratio = count / 735.f;
    static bool normal = true;

    if (ratio < 3.5 && normal) {
        apu.SetSpeedFactor(0.995);
        normal = false;
        if (WARN_SPEED_CHANGE) {
            printf("--- audio slow down\n");
        }
    }
    else if (ratio > 5.5 && !normal) {
        apu.SetSpeedFactor(1);
        normal = true;
        if (WARN_SPEED_CHANGE) {
            printf("+++ audio normal speed\n");
        }
    }
}

void NES::UpdateFrame()
{
    if (state_ == Stopped)
        return;

    if (frame_ % AUDIO_DELAY_FRAME == 0)
        PlaySamples();

    update_audio_speed();

    for (;;) {
        if (need_log()) {
            PrintCpuStatus(cpu, ppu);
            log_line_count_++;
        }

        const int cpu_cycles = cpu.IsSuspended() ? dma.Run() : cpu.Run();
        const bool frame_ready = ppu.Run(cpu_cycles);
        apu.Run(cpu_cycles);
        cart_->Run(cpu_cycles);

        if (frame_ready)
            break;
    }

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
    state_ = Stopped;
}

bool NES::IsRunning() const
{
    return state_ == Running;
}

void NES::Step()
{
    state_ = Stepping;
}

uint64_t NES::GetLogLineCount() const
{
    return log_line_count_;
}

void NES::SetChannelEnable(uint64_t chan_bits)
{
    apu.SetChannelEnable(chan_bits);
}

uint64_t NES::GetChannelEnable() const
{
    return apu.GetChannelEnable();
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

static void print_code(const Code &code)
{
    const std::string code_str = GetCodeString(code);

    printf("    %s\n", code_str.c_str());
}

static void print_current_code(const Code &code, const CPU &cpu)
{
    const std::string code_str = GetCodeString(code);
    const std::string mem_str = GetMemoryString(code, cpu);

    printf(" -> %s%s\n", code_str.c_str(), mem_str.c_str());
}

static int find_nearest_next(const Assembly &assem, uint16_t pc)
{
    for (int i = 0; i < assem.GetCount(); i++) {
        const Code code = assem.GetCode(i);

        if (code.address > pc)
            return i;
    }
    return -1;
}

void NES::print_disassemble() const
{
    printf("==============================\n");
    print_cpu_status(cpu.GetStatus());
    printf("------------------------------\n");

    Assembly assem;
    assem.DisassembleProgram(cpu);

    const int index = assem.FindCode(cpu.GetPC());
    if (index != -1) {
        const int start = std::max(index - 16, 0);
        const int end   = std::min(index + 16, assem.GetCount());

        for (int i = start; i < index; i++)
            print_code(assem.GetCode(i));

        print_current_code(assem.GetCode(index), cpu);

        for (int i = index + 1; i <= end; i++)
            print_code(assem.GetCode(i));
    }
    else {
        const uint16_t pc = cpu.GetPC();
        const int next_index = find_nearest_next(assem, pc);

        const int start = std::max(next_index - 16, 0);
        const int end   = std::min(next_index + 16, assem.GetCount());

        for (int i = start; i < next_index; i++)
            print_code(assem.GetCode(i));

        printf("\033[0;31m");
        print_current_code(DisassembleLine(cpu, pc), cpu);
        printf("\033[0;39m");

        for (int i = next_index; i < end; i++)
            print_code(assem.GetCode(i));
    }
}

} // namespace
