#include "ppu.h"
#include "cartridge.h"

namespace nes {

// --------------------------------------------------------------------------
// status

enum ppu_status {
    STAT_UNUSED          = 0x1F,
    STAT_SPRITE_OVERFLOW = 1 << 5,
    STAT_SPRITE_ZERO_HIT = 1 << 6,
    STAT_VERTICAL_BLANK  = 1 << 7
};

enum ppu_control {
    CTRL_NAMETABLE_X     = 1 << 0,
    CTRL_NAMETABLE_Y     = 1 << 1,
    CTRL_ADDR_INCREMENT  = 1 << 2,
    CTRL_PATTERN_SPRITE  = 1 << 3,
    CTRL_PATTERN_BG      = 1 << 4,
    CTRL_SPRITE_SIZE     = 1 << 5,
    CTRL_SLAVE_MODE      = 1 << 6,
    CTRL_ENABLE_NMI      = 1 << 7
};

enum ppu_mask {
    MASK_GREYSCALE        = 1 << 0,
    MASK_SHOW_BG_LEFT     = 1 << 1,
    MASK_SHOW_SPRITE_LEFT = 1 << 2,
    MASK_SHOW_BG          = 1 << 3,
    MASK_SHOW_SPRITE      = 1 << 4,
    MASK_EMPHASIZE_R      = 1 << 5,
    MASK_EMPHASIZE_G      = 1 << 6,
    MASK_EMPHASIZE_B      = 1 << 7
};

void PPU::set_stat(uint8_t flag, uint8_t val)
{
    if (val)
        stat_ |= flag;
    else
        stat_ &= ~flag;
}

bool PPU::get_ctrl(uint8_t flag) const
{
    return (ctrl_ & flag) > 0;
}

bool PPU::is_rendering_bg() const
{
    return mask_ & MASK_SHOW_BG;
}

bool PPU::is_rendering_sprite() const
{
    return mask_ & MASK_SHOW_SPRITE;
}

bool PPU::is_rendering_left_bg(int x) const
{
    if (x >= 0 && x <= 7)
        return mask_ & MASK_SHOW_BG_LEFT;
    else
        return true;
}

bool PPU::is_rendering_left_sprite(int x) const
{
    if (x >= 0 && x <= 7)
        return mask_ & MASK_SHOW_SPRITE_LEFT;
    else
        return true;
}

void PPU::enter_vblank()
{
    set_stat(STAT_VERTICAL_BLANK, 1);

    if (get_ctrl(CTRL_ENABLE_NMI))
        nmi_generated_ = true;
}

void PPU::leave_vblank()
{
    set_stat(STAT_VERTICAL_BLANK, 0);
}

// --------------------------------------------------------------------------
// color

static const uint8_t palette_2C02[][3] = {
    { 84,  84,  84}, {  0,  30, 116}, {  8,  16, 144}, { 48,   0, 136},
    { 68,   0, 100}, { 92,   0,  48}, { 84,   4,   0}, { 60,  24,   0},
    { 32,  42,   0}, {  8,  58,   0}, {  0,  64,   0}, {  0,  60,   0},
    {  0,  50,  60}, {  0,   0,   0}, {  0,   0,   0}, {  0,   0,   0},

    {152, 150, 152}, {  8,  76, 196}, { 48,  50, 236}, { 92,  30, 228},
    {136,  20, 176}, {160,  20, 100}, {152,  34,  32}, {120,  60,   0},
    { 84,  90,   0}, { 40, 114,   0}, {  8, 124,   0}, {  0, 118,  40},
    { 0 ,102 ,120 }, {  0,   0,   0}, {  0,   0,   0}, {  0,   0,   0},

    {236, 238, 236}, { 76, 154, 236}, {120, 124, 236}, {176,  98, 236},
    {228,  84, 236}, {236,  88, 180}, {236, 106, 100}, {212, 136,  32},
    {160, 170,   0}, {116, 196,   0}, { 76, 208,  32}, { 56, 204, 108},
    {56 ,180 ,204 }, { 60,  60,  60}, {  0,   0,   0}, {  0,   0,   0},

    {236, 238, 236}, {168, 204, 236}, {188, 188, 236}, {212, 178, 236},
    {236, 174, 236}, {236, 174, 212}, {236, 180, 176}, {228, 196, 144},
    {204, 210, 120}, {180, 222, 120}, {168, 226, 144}, {152, 226, 180},
    {160, 214, 228}, {160, 162, 160}, {  0,   0,   0}, {  0,   0,   0}
};

static Color get_color(int index)
{
    const uint8_t *c = palette_2C02[index];
    const Color col = {c[0], c[1], c[2]};
    return col;
}

static uint16_t name_table_index(const Cartridge *cart_, uint16_t addr)
{
    const uint16_t index = addr - 0x2000;

    // vertical mirroring
    if (cart_->IsVerticalMirroring())
        return index & 0x07FF;

    // horizontal mirroring
    if (index >= 0x0000 && index <= 0x07FF)
        return index & 0x03FF;

    if (index >= 0x0800 && index <= 0x0FFF)
        return 0x400 | (index & 0x03FF);

    // unreachable
    return 0x0000;
}

uint8_t PPU::read_byte(uint16_t addr) const
{
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        return cart_->ReadChar(addr);
    }
    else if (addr >= 0x2000 && addr <= 0x2FFF) {
        const uint16_t index = name_table_index(cart_, addr);
        return name_table_[index];
    }
    else if (addr >= 0x3000 && addr <= 0x3EFF) {
        // mirrors of 0x2000-0x2EFF
        return read_byte(addr - 0x1000);
    }
    else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        const uint16_t a = 0x3F00 + (addr & 0x1F);
        // Addresses $3F04/$3F08/$3F0C can contain unique data,
        // though these values are not used by the PPU when normally rendering
        // (since the pattern values that would otherwise select those cells
        // select the backdrop color instead).
        if (a % 4 == 0)
            return palette_ram_[0x00];
        else
            return palette_ram_[a & 0x1F];
    }

    return 0xFF;
}

void PPU::write_byte(uint16_t addr, uint8_t data)
{
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        cart_->WriteChar(addr, data);
    }
    else if (addr >= 0x2000 && addr <= 0x2FFF) {
        const uint16_t index = name_table_index(cart_, addr);
        name_table_[index] = data;
    }
    else if (addr >= 0x3000 && addr <= 0x3EFF) {
        // mirrors of 0x2000-0x2EFF
        write_byte(addr - 0x1000, data);
    }
    else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        // $3F20-$3FFF -> Mirrors of $3F00-$3F1F
        uint16_t a = 0x3F00 + (addr & 0x1F);

        // Addresses $3F10/$3F14/$3F18/$3F1C are mirrors of
        // $3F00/$3F04/$3F08/$3F0C.
        if (addr == 0x3F10) a = 0x3F00;
        if (addr == 0x3F14) a = 0x3F04;
        if (addr == 0x3F18) a = 0x3F08;
        if (addr == 0x3F1C) a = 0x3F0C;

        palette_ram_[a & 0x1F] = data;
    }
}

int PPU::address_increment() const
{
    return get_ctrl(CTRL_ADDR_INCREMENT) ? 32 : 1;
}

// --------------------------------------------------------------------------
// address

struct VramPointer {
    uint8_t table_x = 0, table_y = 0;
    uint8_t tile_x = 0, tile_y = 0;
    uint8_t fine_y = 0;
};

static VramPointer decode_address(uint16_t addr)
{
    // yyy NN YYYYY XXXXX
    // ||| || ||||| +++++-- coarse X scroll
    // ||| || +++++-------- coarse Y scroll
    // ||| ++-------------- nametable select
    // +++----------------- fine Y scroll
    VramPointer v;

    v.tile_x  = (addr     )  & 0x001F;
    v.tile_y  = (addr >> 5)  & 0x001F;
    v.table_x = (addr >> 10) & 0x0001;
    v.table_y = (addr >> 11) & 0x0001;
    v.fine_y  = (addr >> 12) & 0x0007;

    return v;
}

static uint16_t encode_address(VramPointer v)
{
    // yyy NN YYYYY XXXXX
    // ||| || ||||| +++++-- coarse X scroll
    // ||| || +++++-------- coarse Y scroll
    // ||| ++-------------- nametable select
    // +++----------------- fine Y scroll
    uint16_t addr = 0;

    addr = v.fine_y & 0x07;
    addr = (addr << 1) | (v.table_y & 0x01);
    addr = (addr << 1) | (v.table_x & 0x01);
    addr = (addr << 5) | (v.tile_y & 0x1F);
    addr = (addr << 5) | (v.tile_x & 0x1F);

    return addr;
}

static uint16_t increment_scroll_x(uint16_t vram_addr)
{
    VramPointer v = decode_address(vram_addr);

    if (v.tile_x == 31) {
        v.tile_x = 0;
        v.table_x = !v.table_x;
    }
    else {
        v.tile_x++;
    }

    return encode_address(v);
}

static uint16_t increment_scroll_y(uint16_t vram_addr)
{
    VramPointer v = decode_address(vram_addr);

    if (v.fine_y == 7) {
        v.fine_y = 0;

        if (v.tile_y == 29) {
            v.tile_y = 0;
            v.table_y = !v.table_y;
        }
        else {
            v.tile_y++;
        }
    }
    else {
        v.fine_y++;
    }

    return encode_address(v);
}

static uint16_t copy_address_x(uint16_t vram_addr, uint16_t temp_addr)
{
    const VramPointer t = decode_address(temp_addr);
    VramPointer v = decode_address(vram_addr);

    v.table_x = t.table_x;
    v.tile_x  = t.tile_x;

    return encode_address(v);
}

static uint16_t copy_address_y(uint16_t vram_addr, uint16_t temp_addr)
{
    const VramPointer t = decode_address(temp_addr);
    VramPointer v = decode_address(vram_addr);

    v.table_y = t.table_y;
    v.tile_y  = t.tile_y;
    v.fine_y  = t.fine_y;

    return encode_address(v);
}

// --------------------------------------------------------------------------
// tile

uint8_t PPU::fetch_tile_id() const
{
    return read_byte(0x2000 | (vram_addr_ & 0x0FFF));
}

uint8_t PPU::fetch_tile_attr() const
{
    const VramPointer v = decode_address(vram_addr_);
    const uint16_t attr_x = v.tile_x / 4;
    const uint16_t attr_y = v.tile_y / 4;
    const uint16_t offset = attr_y * 8 + attr_x;
    const uint16_t base = 0x2000 + 32 * 32 * v.table_x + 2 * 32 * 32 * v.table_y;
    const uint8_t attr = read_byte(base + 32 * 30 + offset);

    const uint8_t bit_x = v.tile_x % 4 > 1;
    const uint8_t bit_y = v.tile_y % 4 > 1;
    const uint8_t bit = (bit_y << 1) | bit_x;

    return (attr >> (bit * 2)) & 0x03;
}

uint8_t PPU::fetch_tile_row(uint8_t tile_id, uint8_t plane) const
{
    const VramPointer v = decode_address(vram_addr_);
    const uint16_t base = get_ctrl(CTRL_PATTERN_BG) ? 0x1000 : 0x0000;
    const uint16_t addr = base + 16 * tile_id + plane + v.fine_y;

    return read_byte(addr);
}

void PPU::load_next_tile()
{
    // after rendering is done in a scanline,
    // it is necesarry to shift bits for whole row before loading new row
    if (cycle_ >= 321)
        tile_queue_[2] = tile_queue_[1];

    tile_queue_[1] = tile_queue_[0];
}

static void shift_word(uint8_t *left, uint8_t *right)
{
    *left <<= 1;
    *left |= (*right & 0x80) > 0;
    *right <<= 1;
}

static void shift_tile_data(PatternRow &tile_queue1, PatternRow &tile_queue2)
{
    shift_word(&tile_queue2.lo,         &tile_queue1.lo);
    shift_word(&tile_queue2.hi,         &tile_queue1.hi);
    shift_word(&tile_queue2.palette_lo, &tile_queue1.palette_lo);
    shift_word(&tile_queue2.palette_hi, &tile_queue1.palette_hi);
}

static void set_tile_palette(PatternRow &patt, uint8_t palette)
{
    patt.palette_lo = (palette & 0x01) ? 0xFF : 0x00;
    patt.palette_hi = (palette & 0x02) ? 0xFF : 0x00;
}

void PPU::fetch_tile_data()
{
    PatternRow &next = tile_queue_[0];

    switch (cycle_ % 8) {
    case 1:
        // NT byte
        next.id = fetch_tile_id();
        break;

    case 3:
        // AT byte
        set_tile_palette(next, fetch_tile_attr());
        break;

    case 5:
        // Low BG tile byte
        next.lo = fetch_tile_row(next.id, 0);
        break;

    case 7:
        // High BG tile byte
        next.hi = fetch_tile_row(next.id, 8);
        break;

    default:
        break;
    }
}

void PPU::set_sprite_overflow()
{
    set_stat(STAT_SPRITE_OVERFLOW, 1);
}

void PPU::clear_sprite_overflow()
{
    set_stat(STAT_SPRITE_OVERFLOW, 0);
}

// --------------------------------------------------------------------------
// sprite

void PPU::clear_secondary_oam()
{
    // secondary oam_ crear occurs cycle 1 - 64
    if (cycle_ % 8 == 1)
        secondary_oam_[cycle_ / 8] = ObjectAttribute();

    if (cycle_ == 1)
        sprite_count_ = 0;
}

ObjectAttribute PPU::get_sprite(int index) const
{
    ObjectAttribute obj;
    uint8_t attr = 0;

    if (index < 0 || index > 63)
        return obj;

    obj.y  = oam_[4 * index + 0];
    obj.id = oam_[4 * index + 1];
    attr   = oam_[4 * index + 2];
    obj.x  = oam_[4 * index + 3];

    obj.palette   = (attr & 0x03);
    obj.priority  = (attr & 0x20) > 0;
    obj.flipped_h = (attr & 0x40) > 0;
    obj.flipped_v = (attr & 0x80) > 0;

    return obj;
}

static int is_sprite_visible(ObjectAttribute obj, int scanline, int height)
{
    const int diff = scanline - ((int) obj.y);

    return diff >= 0 && diff < height;
}

void PPU::evaluate_sprite()
{
    // secondary oam crear occurs cycle 65 - 256
    // index = 0 .. 191
    const int index = cycle_ - 65;
    const int sprite_height = 8;

    if (index > 63)
        return;

    if (sprite_count_ > 8)
        return;

    const ObjectAttribute obj = get_sprite(index);

    if (is_sprite_visible(obj, scanline_, sprite_height)) {
        if (sprite_count_ < 8)
            secondary_oam_[sprite_count_] = obj;
        else
            set_sprite_overflow();

        sprite_count_++;
    }
}

uint8_t PPU::fetch_sprite_row(uint8_t tile_id, int y, uint8_t plane) const
{
    const uint16_t base = get_ctrl(CTRL_PATTERN_SPRITE) ? 0x1000 : 0x0000;
    const uint16_t addr = base + 16 * tile_id + plane + y;

    return read_byte(addr);
}

static uint8_t flip_pattern_row(uint8_t bits)
{
    uint8_t src = bits;
    uint8_t dst = 0x00;

    for (int i = 0; i < 8; i++) {
        dst = (dst << 1) | (src & 0x01);
        src >>= 1;
    }

    return dst;
}

void PPU::fetch_sprite_data()
{
    // fetch sprite data occurs cycle 257 - 320
    // index = (0 .. 63) / 8 => 0 .. 7
    const int index = (cycle_ - 257) / 8;
    const int is_visible = index < sprite_count_;
    const int sprite_id = secondary_oam_[index].id;
    int sprite_y = scanline_ - secondary_oam_[index].y;
    PatternRow &patt = rendering_sprite_[index];
    const ObjectAttribute obj = rendering_oam_[index];

    switch (cycle_ % 8) {
    case 3:
        // Attribute and X position
        rendering_oam_[index] = secondary_oam_[index];

        if (is_visible)
            set_tile_palette(patt, rendering_oam_[index].palette);
        else
            set_tile_palette(patt, 0x00);
        break;

    case 5:
        // Low sprite byte
        if (obj.flipped_v)
            sprite_y = 7 - sprite_y;

        if (is_visible)
            patt.lo = fetch_sprite_row(sprite_id, sprite_y, 0);
        else
            patt.lo = 0x00;

        if (obj.flipped_h)
            patt.lo = flip_pattern_row(patt.lo);
        break;

    case 7:
        // High sprite byte
        if (obj.flipped_v)
            sprite_y = 7 - sprite_y;

        if (is_visible)
            patt.hi = fetch_sprite_row(sprite_id, sprite_y, 8);
        else
            patt.hi = 0x00;

        if (obj.flipped_h)
            patt.hi = flip_pattern_row(patt.hi);
        break;

    default:
        break;
    }
}

void PPU::shift_sprite_data()
{
    for (int i = 0; i < 8; i++) {
        ObjectAttribute &obj = rendering_oam_[i];
        PatternRow &patt = rendering_sprite_[i];

        if (obj.x > 0) {
            obj.x--;
        } else {
            patt.lo <<= 1;
            patt.hi <<= 1;
            patt.palette_lo <<= 1;
            patt.palette_hi <<= 1;
        }
    }
}

// --------------------------------------------------------------------------
// rendering

static const Pixel BACKDROP;

static Pixel get_pixel(PatternRow patt, uint8_t fine_x)
{
    const uint8_t mask_ = 0x80 >> fine_x;
    const uint8_t hi = (patt.hi & mask_) > 0;
    const uint8_t lo = (patt.lo & mask_) > 0;
    const uint8_t val = (hi << 1) | lo;
    const uint8_t pal_lo = (patt.palette_lo & mask_) > 0;
    const uint8_t pal_hi = (patt.palette_hi & mask_) > 0;
    const uint8_t pal = (pal_hi << 1) | pal_lo;

    Pixel pix;

    pix.value = val;
    pix.palette = pal;

    return pix;
}

int PPU::is_sprite_zero(ObjectAttribute obj) const
{
    const int sprite_zero_id = oam_[1];

    return obj.id == sprite_zero_id;
}

Pixel PPU::get_pixel_bg() const
{
    return get_pixel(tile_queue_[2], fine_x_);
}

Pixel PPU::get_pixel_fg() const
{
    for (int i = 0; i < 8; i++) {
        const ObjectAttribute obj = rendering_oam_[i];

        if (obj.x == 0) {
            Pixel pix = get_pixel(rendering_sprite_[i], 0);

            pix.palette += 4;
            pix.priority = obj.priority;
            pix.sprite_zero = is_sprite_zero(obj);

            if (pix.value > 0)
                return pix;
        }
    }

    return BACKDROP;
}

static Pixel composite_pixels(Pixel bg, Pixel fg)
{
    if (bg.value == 0 && fg.value == 0)
        return BACKDROP;
    else if (bg.value > 0 && fg.value == 0)
        return bg;
    else if (bg.value == 0 && fg.value > 0)
        return fg;
    else
        return fg.priority == 0 ? fg : bg;
}

bool PPU::is_clipping_left() const
{
    return !(mask_ & MASK_SHOW_BG_LEFT) ||
           !(mask_ & MASK_SHOW_SPRITE_LEFT);
}

bool PPU::is_sprite_zero_hit(Pixel bg, Pixel fg, int x) const
{
    if (!fg.sprite_zero)
        return 0;

    if (!is_rendering_bg() || !is_rendering_sprite())
        return 0;

    if ((x >= 0 && x <= 7) && is_clipping_left())
        return 0;

    if (x == 255)
        return 0;

    if (fg.value == 0 || bg.value == 0)
        return 0;

    if (stat_ & STAT_SPRITE_ZERO_HIT)
        return 0;

    return 1;
}

Color PPU::lookup_pixel_color(Pixel pix) const
{
    const uint16_t addr = 0x3F00 + 4 * pix.palette + pix.value;
    const uint8_t index = read_byte(addr);
    return get_color(index);
}

void PPU::render_pixel(int x, int y)
{
    Pixel bg, fg, out;
    Color col;

    if (is_rendering_bg() && is_rendering_left_bg(x))
        bg = get_pixel_bg();

    if (is_rendering_sprite() && y != 0 && is_rendering_left_sprite(x))
        fg = get_pixel_fg();

    out = composite_pixels(bg, fg);
    col = lookup_pixel_color(out);

    if (!is_rendering_bg() && !is_rendering_sprite() &&
        vram_addr_ >= 0x3F00 && vram_addr_ <= 0x3FFF) {
        const uint8_t index = read_byte(vram_addr_);
        col = get_color(index);
    }

    fbuf_.SetColor(x, y, col);

    if (is_sprite_zero_hit(bg, fg, x))
        set_stat(STAT_SPRITE_ZERO_HIT, 1);
}

void PPU::SetCartride(Cartridge *cart)
{
    cart_ = cart;
}

// --------------------------------------------------------------------------
// clock

void PPU::ClearNMI()
{
    nmi_generated_ = false;
}

bool PPU::IsSetNMI() const
{
    return nmi_generated_;
}

bool PPU::IsFrameReady() const
{
    return cycle_ == 0 && scanline_ == 0;
}

void PPU::Clock()
{
    const bool is_rendering = is_rendering_bg() || is_rendering_sprite();

    if ((scanline_ >= 0 && scanline_ <= 239) || scanline_ == 261) {

        // fetch bg tile
        if ((cycle_ >= 1 && cycle_ <= 256) || (cycle_ >= 321 && cycle_ <= 337)) {

            if (cycle_ % 8 == 0)
                if (is_rendering)
                    vram_addr_ = increment_scroll_x(vram_addr_);

            if (cycle_ % 8 == 1)
                load_next_tile();

            fetch_tile_data();
        }

        // inc vert(v)
        if (cycle_ == 256)
            if (is_rendering)
                vram_addr_ = increment_scroll_y(vram_addr_);

        // hori(v) = hori(t)
        if (cycle_ == 257)
            if (is_rendering)
                vram_addr_ = copy_address_x(vram_addr_, temp_addr_);

        // vert(v) = vert(t)
        if ((cycle_ >= 280 && cycle_ <= 304) && scanline_ == 261)
            if (is_rendering)
                vram_addr_ = copy_address_y(vram_addr_, temp_addr_);

        // clear secondary oam
        if ((cycle_ >= 1 && cycle_ <= 64) && scanline_ != 261)
            clear_secondary_oam();

        // evaluate sprite for next scanline
        if ((cycle_ >= 65 && cycle_ <= 256) && scanline_ != 261)
            evaluate_sprite();

        // fetch sprite
        if (cycle_ >= 257 && cycle_ <= 320)
            fetch_sprite_data();
    }

    if (scanline_ == 241)
        if (cycle_ == 1)
            enter_vblank();

    if (scanline_ == 261) {
        if (cycle_ == 1) {
            leave_vblank();
            clear_sprite_overflow();
            set_stat(STAT_SPRITE_ZERO_HIT, 0);
        }
    }

    // render pixel
    if (scanline_ >= 0 && scanline_ <= 239) {
        if (cycle_ >= 1 && cycle_ <= 256) {
            render_pixel(cycle_ - 1, scanline_);

            if (is_rendering_bg())
                shift_tile_data(tile_queue_[1], tile_queue_[2]);

            if (is_rendering_sprite())
                shift_sprite_data();
        }
    }

    // advance cycle and scanline
    if (cycle_ == 339) {
        if (scanline_ == 261 && frame_ % 2 == 0) {
            // the end of the scanline for odd frames
            cycle_ = 0;
            scanline_ = 0;
            frame_++;
        }
        else {
            cycle_++;
        }
    }
    else if (cycle_ == 340) {
        if (scanline_ == 261) {
            cycle_ = 0;
            scanline_ = 0;
            frame_++;
        }
        else {
            cycle_ = 0;
            scanline_++;
        }
    }
    else {
        cycle_++;
    }
}

void PPU::PowerUp()
{
    ctrl_ = 0x00;
    mask_ = 0x00;
    stat_ = 0xA0;
    oam_addr_ = 0x00;

    write_toggle_ = false;

    temp_addr_ = 0x0000;
    vram_addr_ = 0x0000;
    read_buffer_ = 0x00;

    frame_ = 0;

    for (auto &i : oam_)
        i = 0xFF;

    for (auto &i : palette_ram_)
        i = 0xFF;

    for (auto &i : name_table_)
        i = 0xFF;
}

void PPU::Reset()
{
    ctrl_ = 0x00;
    mask_ = 0x00;
    stat_ &= 0x80;

    write_toggle_ = false;

    temp_addr_   = 0x0000;
    read_buffer_ = 0x00;

    frame_ = 0;

    for (auto &i : oam_)
        i = 0xFF;
}

void PPU::WriteControl(uint8_t data)
{
    // Nametable x and y from control
    // t: ...GH.. ........ <- d: ......GH
    //    <used elsewhere> <- d: ABCDEF..
    ctrl_ = data;
    temp_addr_ = (temp_addr_ & 0xF3FF) | ((data & 0x03) << 10);
}

void PPU::WriteMask(uint8_t data)
{
    mask_ = data;
}

void PPU::WriteOamAddress(uint8_t addr)
{
    oam_addr_ = addr;
}

void PPU::WriteOamData(uint8_t data)
{
    oam_[oam_addr_] = data;
}

void PPU::WriteScroll(uint8_t data)
{
    if (write_toggle_ == false) {
        // Coarse X and fine x
        // t: ....... ...ABCDE <- d: ABCDE...
        // x:              FGH <- d: .....FGH
        // w:                  <- 1
        fine_x_ = data & 0x07;
        temp_addr_ = (temp_addr_ & 0xFFE0) | (data >> 3);
        write_toggle_ = true;
    } else {
        // Coarse Y and fine y
        // t: FGH..AB CDE..... <- d: ABCDEFGH
        // w:                  <- 0
        temp_addr_ = (temp_addr_ & 0xFC1F) | ((data >> 3) << 5);
        temp_addr_ = (temp_addr_ & 0x8FFF) | ((data & 0x07) << 12);
        write_toggle_ = false;
    }
}

void PPU::WriteAddress(uint8_t addr)
{
    if (write_toggle_ == false) {
        // High byte
        // t: .CDEFGH ........ <- d: ..CDEFGH
        //        <unused>     <- d: AB......
        // t: Z...... ........ <- 0 (bit Z is cleared)
        // w:                  <- 1
        temp_addr_ = (temp_addr_ & 0xC0FF) | ((addr & 0x3F) << 8);
        temp_addr_ &= 0xBFFF;
        write_toggle_ = true;
    } else {
        // Low byte
        // t: ....... ABCDEFGH <- d: ABCDEFGH
        // v: <...all bits...> <- t: <...all bits...>
        // w:                  <- 0
        temp_addr_ = (temp_addr_ & 0xFF00) | addr;
        vram_addr_ = temp_addr_;
        write_toggle_ = false;
    }
}

void PPU::WriteData(uint8_t data)
{
    write_byte(vram_addr_, data);

    vram_addr_ += address_increment();
}

uint8_t PPU::ReadStatus()
{
    const uint8_t data = stat_;

    set_stat(STAT_VERTICAL_BLANK, 0);
    write_toggle_ = false;

    return data;
}

uint8_t PPU::ReadOamData() const
{
    return oam_[oam_addr_];
}

uint8_t PPU::ReadData()
{
    const uint16_t addr = vram_addr_;
    uint8_t data = read_buffer_;

    read_buffer_ = read_byte(addr);

    if (addr >= 0x3F00 && addr <= 0x3FFF)
        data = read_buffer_;

    vram_addr_ += address_increment();

    return data;
}

uint8_t PPU::PeekStatus() const
{
    return stat_;
}

void PPU::WriteDmaSprite(uint8_t addr, uint8_t data)
{
    oam_[addr] = data;
}

ObjectAttribute PPU::ReadOam(int index) const
{
    return get_sprite(index);
}

} // namespace
