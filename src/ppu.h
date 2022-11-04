#ifndef PPU_H
#define PPU_H

#include <cstdint>

namespace nes {

class FrameBuffer;
class Cartridge;

struct PatternRow {
    uint8_t id = 0;
    uint8_t lo = 0, hi = 0;
    uint8_t palette_lo = 0;
    uint8_t palette_hi = 0;
};

struct ObjectAttribute {
    uint8_t id = 0xFF;
    uint8_t x, y = 0xFF;
    uint8_t palette = 0xFF;
    uint8_t priority = 0xFF;
    uint8_t flipped_h = 0xFF;
    uint8_t flipped_v = 0xFF;
};

class PPU {
public:
    PPU() {}
    ~PPU() {}

    Cartridge *cart;

    // registers
    uint8_t ctrl = 0;
    uint8_t mask = 0;
    uint8_t stat = 0;

    // vram and scroll
    uint8_t write_toggle = 0;
    uint16_t vram_addr = 0;
    uint16_t temp_addr = 0;
    uint8_t fine_x = 0;

    // vram
    uint8_t read_buffer = 0;
    uint8_t palette_ram[32] = {0};
    uint8_t name_table[2048] = {0};

    // bg tile cache
    PatternRow tile_queue[3];

    // fg sprite
    uint8_t oam_addr = 0;
    uint8_t oam[256] = {0};
    ObjectAttribute secondary_oam[8];
    // 8 latches and 8 counters
    ObjectAttribute rendering_oam[8];
    PatternRow rendering_sprite[8];
    int sprite_count = 0;

    int cycle = 0;
    int scanline = 0;
    uint64_t frame = 0;
    FrameBuffer *fbuf = nullptr;

    bool nmi_generated = false;

    // interruptions
    void ClearNMI();
    bool IsSetNMI() const;
    bool IsFrameReady() const;

    // clock
    void Clock();
    void PowerUp();
    void Reset();

    // write registers
    void WriteControl(uint8_t data);
    void WriteMask(uint8_t data);
    void WriteOamAddress(uint8_t addr);
    void WriteOamData(uint8_t data);
    void WriteScroll(uint8_t data);
    void WriteAddress(uint8_t addr);
    void WriteData(uint8_t data);

    // read registers
    uint8_t ReadStatus();
    uint8_t ReadOamData() const;
    uint8_t ReadData();

    // peek registers
    uint8_t PeekStatus() const;
    // sprites
    void WriteDmaSprite(uint8_t addr, uint8_t data);
    // debug
    ObjectAttribute ReadOam(int index) const;

//private:
    void set_stat(uint8_t flag, uint8_t val);
    int get_ctrl(uint8_t flag) const;
    bool is_rendering_bg() const;
    bool is_rendering_sprite() const;
    void enter_vblank();
    void leave_vblank();
    void set_sprite_overflow();

    uint8_t read_byte(uint16_t addr) const;
    void write_byte(uint16_t addr, uint8_t data);
};

} // namespace

#endif // _H
