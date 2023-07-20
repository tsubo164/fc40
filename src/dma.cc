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
            // The DMA transfer will begin at the current OAM write address.
            // It is common practice to initialize it to 0 with a write to
            // OAMADDR before the DMA transfer.
            page_ = ppu_.PeekOamDma();
            addr_ = ppu_.PeekOamAddr();
            write_count_ = 0;
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
        write_count_++;

        if (write_count_ == 256) {
            wait_ = true;
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
    page_ = 0;
    addr_ = 0;
    data_ = 0;
    write_count_ = 0;
}

} // namespace
