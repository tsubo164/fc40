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

#define SERIALIZE_NAMESPACE_BEGIN(ar, space) (ar).EnterNamespcae((space))
#define SERIALIZE_NAMESPACE_END(ar) (ar).LeaveNamespcae()
#define SERIALIZE(ar, obj, member) Serialize((ar),#member,&(obj)->member)
    friend
    void Serialize(Archive &ar, const std::string &name, DMA *dma)
    {
        SERIALIZE_NAMESPACE_BEGIN(ar, name);
        SERIALIZE(ar, dma, cycles_);
        SERIALIZE(ar, dma, wait_);
        SERIALIZE(ar, dma, page_);
        SERIALIZE(ar, dma, addr_);
        SERIALIZE(ar, dma, data_);
        SERIALIZE(ar, dma, write_count_);
        SERIALIZE_NAMESPACE_END(ar);
    }
};

} // namespace

#endif // _H
