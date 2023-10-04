#ifndef BANK_MAP_H
#define BANK_MAP_H

#include <cstdint>

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

    void Resize(int rom_size, Size bank_size, int window_count)
    {
        rom_size_ = rom_size;
        bank_size_ = static_cast<int>(bank_size);
        bank_count_ = rom_size_ / bank_size_;

        windows_.resize(window_count);
        for (int i = 0; i < windows_.size(); i++)
            windows_[i] = i;
    }

    void Select(int window_index, int bank_index)
    {
        if (bank_index < 0)
            windows_[window_index] = bank_count_ + bank_index;
        else
            // does mirroring
            windows_[window_index] = bank_index % bank_count_;
    }

    int Map(int addr) const
    {
        const int offset = addr % bank_size_;
        const int window = addr / bank_size_;
        const int base = windows_[window] * bank_size_;
        return base + offset;
    }

    int GetBankSize() const
    {
        return bank_size_;
    }

private:
    std::vector<int> windows_;
    int rom_size_ = 0;
    int bank_size_ = 0;
    int bank_count_ = 0;
};

#endif // _H
