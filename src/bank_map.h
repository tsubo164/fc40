#ifndef BANK_MAP_H
#define BANK_MAP_H

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
class bankmap {
public:
    bankmap()
    {
        for (int i = 0; i < static_cast<int>(windows_.size()); i++)
            windows_[i] = i;
    }
    ~bankmap() {}

    void resize(int capacity)
    {
        if (capacity > 0)
            bank_count_ = capacity / static_cast<int>(BANK_SIZE);
        else
            bank_count_ = 1;
    }

    void select(int window_index, int bank_index)
    {
        if (bank_index < 0)
            windows_[window_index] = bank_count_ + bank_index;
        else
            // does mirroring
            windows_[window_index] = bank_index % bank_count_;
    }

    int map(int addr) const
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

private:
    std::array<int,WINDOW_COUNT> windows_;
    int bank_count_ = 1;
};

#endif // _H
