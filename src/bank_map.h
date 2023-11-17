#ifndef BANK_MAP_H
#define BANK_MAP_H

#include "serialize.h"

namespace nes {

enum class Size {
    _1KB   = 0x400 << 0,
    _2KB   = 0x400 << 1,
    _4KB   = 0x400 << 2,
    _8KB   = 0x400 << 3,
    _16KB  = 0x400 << 4,
    _32KB  = 0x400 << 5,
    _64KB  = 0x400 << 6,
    _128KB = 0x400 << 7,
    _256KB = 0x400 << 8,
    _512KB = 0x400 << 9,
};

template<Size BANK_SIZE, int WINDOW_COUNT>
class bank_map {
public:
    void resize(int capacity)
    {
        if (capacity > 0)
            bank_count_ = capacity / static_cast<int>(BANK_SIZE);
        else
            bank_count_ = 1;

        for (int i = 0; i < static_cast<int>(windows_.size()); i++)
            select(i, i);
    }

    void select(int window_index, int bank_index)
    {
        if (bank_index < 0)
            windows_[window_index] = bank_count_ + bank_index;
        else
            // does mirroring
            windows_[window_index] = bank_index % bank_count_;
    }

    int map(uint16_t addr) const
    {
        const int offset = addr % static_cast<int>(BANK_SIZE);
        const int window = addr / static_cast<int>(BANK_SIZE);
        const int base = windows_[window] * static_cast<int>(BANK_SIZE);
        return base + offset;
    }

    int bank(int window_index) const
    {
        if (window_index >= 0 && window_index < static_cast<int>(windows_.size()))
            return windows_[window_index];
        else
            return 0;
    }

    int bank_count() const
    {
        return bank_count_;
    }

    constexpr int window_count() const
    {
        return WINDOW_COUNT;
    }

private:
    std::array<int,WINDOW_COUNT> windows_ = {0};
    int bank_count_ = 1;

    // serialization
    friend void Serialize(Archive &ar, const std::string &name,
            bank_map<BANK_SIZE, WINDOW_COUNT> *data)
    {
        SERIALIZE_NAMESPACE_BEGIN(ar, name);
        SERIALIZE(ar, data, windows_);
        SERIALIZE(ar, data, bank_count_);
        SERIALIZE_NAMESPACE_END(ar);
    }
};

struct BankInfo {
    std::vector<int> selected;
    int bank_count;
};

template<Size BANK_SIZE, int WINDOW_COUNT>
void GetBankInfo(const bank_map<BANK_SIZE, WINDOW_COUNT> &map, BankInfo &info)
{
    const int N = map.window_count();
    info.selected.resize(N);

    for (int i = 0; i < N; i++) {
        info.selected[i] = map.bank(i);
    }

    info.bank_count = map.bank_count();
}

inline
void GetDefaultBankInfo(BankInfo &info)
{
    info.selected.resize(1);
    info.selected[0] = 0;

    info.bank_count = 1;
}

} // namespace

#endif // _H
