#ifndef DMA_H
#define DMA_H

#include <cstdint>
#include "serialize.h"

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
    uint8_t page_ = 0;
    uint8_t addr_ = 0;
    uint8_t data_ = 0;
    int write_count_ = 0;

    // serialization
    friend void Serialize(Archive &ar, const std::string &name, DMA *data)
    {
        SERIALIZE_NAMESPACE_BEGIN(ar, name);
        SERIALIZE(ar, data, cycles_);
        SERIALIZE(ar, data, wait_);
        SERIALIZE(ar, data, page_);
        SERIALIZE(ar, data, addr_);
        SERIALIZE(ar, data, data_);
        SERIALIZE(ar, data, write_count_);
        SERIALIZE_NAMESPACE_END(ar);
    }
};

} // namespace

#endif // _H
