#ifndef BANK_MAP_H
#define BANK_MAP_H

#include <cstdint>
#include <iostream>

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

class BankMap {
public:
    BankMap() {}
    ~BankMap() {}

    void Resize(uint32_t rom_size, Size window_size, Size bank_size)
    {
        rom_size_ = rom_size;
        bank_size_ = static_cast<uint32_t>(bank_size);
        bank_count_ = rom_size_ / bank_size_;

        const uint16_t window_count =
            static_cast<uint32_t>(window_size) / bank_size_;

        windows_.resize(window_count);
        for (int i = 0; i < windows_.size(); i++)
            windows_[i] = i;
    }

    void Select(uint16_t window_index, int32_t bank_index)
    {
        if (bank_index < 0)
            windows_[window_index] = bank_count_ + bank_index;
        else
            // does mirroring
            windows_[window_index] = bank_index % bank_count_;
    }

    uint32_t Map(uint16_t addr16) const
    {
        const uint32_t offset = addr16 % bank_size_;
        const uint32_t window = addr16 / bank_size_;
        const uint32_t base = windows_[window] * bank_size_;
        return base + offset;
    }

private:
    std::vector<uint16_t> windows_;
    uint32_t rom_size_ = 0;
    uint16_t bank_size_ = 0;
    uint16_t bank_count_ = 0;
};

#endif // _H
