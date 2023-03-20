#ifndef PPU_H
#define PPU_H

#include <cstdint>
#include "framebuffer.h"

namespace nes {

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

struct Pixel {
    uint8_t value = 0;
    uint8_t palette = 0;
    uint8_t priority = 0;
    bool sprite_zero = false;
};

class PPU {
public:
    PPU(FrameBuffer &fb) : fbuf_(fb) {}
    ~PPU() {}

    void SetCartride(Cartridge *cart);

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
    int GetCycle() const;
    int GetScanline() const;

private:
    Cartridge *cart_ = nullptr;

    // registers
    uint8_t ctrl_ = 0;
    uint8_t mask_ = 0;
    uint8_t stat_ = 0;

    // vram and scroll
    bool write_toggle_ = 0;
    uint16_t vram_addr_ = 0;
    uint16_t temp_addr_ = 0;
    uint8_t fine_x_ = 0;

    // vram
    uint8_t read_buffer_ = 0;
    uint8_t palette_ram_[32] = {0};
    uint8_t name_table_[2048] = {0};

    // bg tile cache
    PatternRow tile_queue_[3];

    // fg sprite
    uint8_t oam_addr_ = 0;
    uint8_t oam_[256] = {0};
    ObjectAttribute secondary_oam_[8];
    // 8 latches and 8 counters
    ObjectAttribute rendering_oam_[8];
    PatternRow rendering_sprite_[8];
    int sprite_count_ = 0;

    int cycle_ = 0;
    int scanline_ = 0;
    uint64_t frame_ = 0;
    FrameBuffer &fbuf_;

    bool nmi_generated_ = false;

    // control
    void set_stat(uint8_t flag, uint8_t val);
    bool get_ctrl(uint8_t flag) const;
    bool is_rendering_bg() const;
    bool is_rendering_sprite() const;
    bool is_rendering_left_bg(int x) const;
    bool is_rendering_left_sprite(int x) const;
    void enter_vblank();
    void leave_vblank();
    int address_increment() const;

    uint8_t read_byte(uint16_t addr) const;
    void write_byte(uint16_t addr, uint8_t data);

    // tile
    uint8_t fetch_tile_id() const;
    uint8_t fetch_tile_attr()const; 
    uint8_t fetch_tile_row(uint8_t tile_id, uint8_t plane) const;
    void load_next_tile();
    void fetch_tile_data();

    void set_sprite_overflow();
    void clear_sprite_overflow();

    // sprite
    void clear_secondary_oam();
    ObjectAttribute get_sprite(int index) const;
    void evaluate_sprite();
    uint8_t fetch_sprite_row(uint8_t tile_id, int y, uint8_t plane) const;
    void fetch_sprite_data();
    void shift_sprite_data();

    // rendering
    int is_sprite_zero(ObjectAttribute obj) const;
    Pixel get_pixel_bg() const;
    Pixel get_pixel_fg() const;
    bool is_clipping_left() const;
    bool is_sprite_zero_hit(Pixel bg, Pixel fg, int x) const;
    Color lookup_pixel_color(Pixel pix) const;
    void render_pixel(int x, int y);
};

} // namespace

#endif // _H
