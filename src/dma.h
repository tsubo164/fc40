#ifndef DMA_H
#define DMA_H

#include <cstdint>

namespace nes {

class CPU;
class PPU;

class DMA {
public:
    DMA(CPU &cpu, PPU &ppu);
    ~DMA();

    int Run();
    void Clock();
    void PowerUp();
    void Reset();

private:
    CPU &cpu_;
    PPU &ppu_;

    uint64_t cycles_ = 0;

    bool wait_ = true;
    uint8_t addr_ = 0;
    uint8_t data_ = 0;
};

} // namespace

#endif // _H
