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

    uint8_t nmi_generated = 0;

    // interruptions
    void ClearNMI();
    bool IsSetNMI() const;
    bool IsFrameReady() const;

    // clock
    void Clock();
    void PowerUp();
    void Reset();
};

// write registers
extern void write_ppu_control(PPU *ppu, uint8_t data);
extern void write_ppu_mask(PPU *ppu, uint8_t data);
extern void write_oam_address(PPU *ppu, uint8_t addr);
extern void write_oam_data(PPU *ppu, uint8_t data);
extern void write_ppu_scroll(PPU *ppu, uint8_t data);
extern void write_ppu_address(PPU *ppu, uint8_t addr);
extern void write_ppu_data(PPU *ppu, uint8_t data);

// read registers
extern uint8_t read_ppu_status(PPU *ppu);
extern uint8_t read_oam_data(const PPU *ppu);
extern uint8_t read_ppu_data(PPU *ppu);

// peek registers
extern uint8_t peek_ppu_status(const PPU *ppu);

// sprites
extern void write_dma_sprite(PPU *ppu, uint8_t addr, uint8_t data);

// debug
extern ObjectAttribute read_oam(const PPU *ppu, int index);

} // namespace

#endif // _H
