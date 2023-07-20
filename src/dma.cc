#include "dma.h"
#include "cpu.h"
#include "ppu.h"

namespace nes {

DMA::DMA(CPU &cpu, PPU &ppu) : cpu_(cpu), ppu_(ppu)
{
}

DMA::~DMA()
{
}

int DMA::Run()
{
    if (!cpu_.IsSuspended())
        return 0;

    // Run one read/write cycles at a time
    // Decouple from the current cpu cycles for simplification
    const int dma_cycles = 2;

    for (int i = 0; i < dma_cycles; i++)
        Clock();

    return dma_cycles;
}

void DMA::Clock()
{
    if (wait_) {
        // Idle for this cpu cycle
        if (cycles_ % 2 == 1) {
            wait_ = false;
            addr_ = 0x00;
            page_ = cpu_.GetDmaPage();
        }
    }
    else if (cycles_ % 2 == 0) {
        // Read
        data_ = cpu_.PeekData((page_ << 8) | addr_);
    }
    else if (cycles_ % 2 == 1) {
        // Write
        ppu_.WriteDmaSprite(addr_, data_);
        addr_++;

        if (addr_ == 0x00) {
            wait_ = true;
            page_ = 0x00;
            addr_ = 0x00;
            cpu_.Resume();
        }
    }

    cycles_++;
}

void DMA::PowerUp()
{
    Reset();
}

void DMA::Reset()
{
    wait_ = true;
    addr_ = 0;
    page_ = 0;
    data_ = 0;
}

} // namespace
