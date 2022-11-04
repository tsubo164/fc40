#include <cstdlib>
#include "ppu.h"
#include "framebuffer.h"
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
        stat |= flag;
    else
        stat &= ~flag;
}

int PPU::get_ctrl(uint8_t flag) const
{
    return (ctrl & flag) > 0;
}

bool PPU::is_rendering_bg() const
{
    return mask & MASK_SHOW_BG;
}

bool PPU::is_rendering_sprite() const
{
    return mask & MASK_SHOW_SPRITE;
}

void PPU::enter_vblank()
{
    set_stat(STAT_VERTICAL_BLANK, 1);

    if (get_ctrl(CTRL_ENABLE_NMI))
        nmi_generated = true;
}

void PPU::leave_vblank()
{
    set_stat(STAT_VERTICAL_BLANK, 0);
}

void PPU::set_sprite_overflow()
{
    set_stat(STAT_SPRITE_OVERFLOW, 1);
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

static struct Color get_color(int index)
{
    const uint8_t *c = palette_2C02[index];
    const struct Color col = {c[0], c[1], c[2]};
    return col;
}

static uint16_t name_table_index(const Cartridge *cart, uint16_t addr)
{
    const uint16_t index = addr - 0x2000;

    /* vertical mirroring */
    if (cart->IsVerticalMirroring())
        return index & 0x07FF;

    /* horizontal mirroring */
    if (index >= 0x0000 && index <= 0x07FF)
        return index & 0x03FF;

    if (index >= 0x0800 && index <= 0x0FFF)
        return 0x400 | (index & 0x03FF);

    /* unreachable */
    return 0x0000;
}

uint8_t PPU::read_byte(uint16_t addr) const
{
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        return cart->ReadChar(addr);
    }
    else if (addr >= 0x2000 && addr <= 0x2FFF) {
        const uint16_t index = name_table_index(cart, addr);
        return name_table[index];
    }
    else if (addr >= 0x3000 && addr <= 0x3EFF) {
        /* mirrors of 0x2000-0x2EFF */
        return read_byte(addr - 0x1000);
    }
    else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        const uint16_t a = 0x3F00 + (addr & 0x1F);
        /* Addresses $3F04/$3F08/$3F0C can contain unique data,
         * though these values are not used by the PPU when normally rendering
         * (since the pattern values that would otherwise select those cells
         * select the backdrop color instead). */
        if (a % 4 == 0)
            return palette_ram[0x00];
        else
            return palette_ram[a & 0x1F];
    }

    return 0xFF;
}

void PPU::write_byte(uint16_t addr, uint8_t data)
{
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        cart->WriteChar(addr, data);
    }
    else if (addr >= 0x2000 && addr <= 0x2FFF) {
        const uint16_t index = name_table_index(cart, addr);
        name_table[index] = data;
    }
    else if (addr >= 0x3000 && addr <= 0x3EFF) {
        /* mirrors of 0x2000-0x2EFF */
        write_byte(addr - 0x1000, data);
    }
    else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        /* $3F20-$3FFF -> Mirrors of $3F00-$3F1F */
        uint16_t a = 0x3F00 + (addr & 0x1F);

        /* Addresses $3F10/$3F14/$3F18/$3F1C are mirrors of
         * $3F00/$3F04/$3F08/$3F0C. */
        if (addr == 0x3F10) a = 0x3F00;
        if (addr == 0x3F14) a = 0x3F04;
        if (addr == 0x3F18) a = 0x3F08;
        if (addr == 0x3F1C) a = 0x3F0C;

        palette_ram[a & 0x1F] = data;
    }
}

/* -------------------------------------------------------------------------- */
/* address */

struct vram_pointer {
    uint8_t table_x, table_y;
    uint8_t tile_x, tile_y;
    uint8_t fine_y;
};

static struct vram_pointer decode_address(uint16_t addr)
{
    /*
     *  yyy NN YYYYY XXXXX
     *  ||| || ||||| +++++-- coarse X scroll
     *  ||| || +++++-------- coarse Y scroll
     *  ||| ++-------------- nametable select
     *  +++----------------- fine Y scroll
     */
    struct vram_pointer v;

    v.tile_x  = (addr     )  & 0x001F;
    v.tile_y  = (addr >> 5)  & 0x001F;
    v.table_x = (addr >> 10) & 0x0001;
    v.table_y = (addr >> 11) & 0x0001;
    v.fine_y  = (addr >> 12) & 0x0007;

    return v;
}

static uint16_t encode_address(struct vram_pointer v)
{
    /*
     *  yyy NN YYYYY XXXXX
     *  ||| || ||||| +++++-- coarse X scroll
     *  ||| || +++++-------- coarse Y scroll
     *  ||| ++-------------- nametable select
     *  +++----------------- fine Y scroll
     */
    uint16_t addr = 0;

    addr = v.fine_y & 0x07;
    addr = (addr << 1) | (v.table_y & 0x01);
    addr = (addr << 1) | (v.table_x & 0x01);
    addr = (addr << 5) | (v.tile_y & 0x1F);
    addr = (addr << 5) | (v.tile_x & 0x1F);

    return addr;
}

static void increment_scroll_x(PPU *ppu)
{
    struct vram_pointer v = decode_address(ppu->vram_addr);

    if (v.tile_x == 31) {
        v.tile_x = 0;
        v.table_x = !v.table_x;
    }
    else {
        v.tile_x++;
    }

    ppu->vram_addr = encode_address(v);
}

static void increment_scroll_y(PPU *ppu)
{
    struct vram_pointer v = decode_address(ppu->vram_addr);

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

    ppu->vram_addr = encode_address(v);
}

static void copy_address_x(PPU *ppu)
{
    const struct vram_pointer t = decode_address(ppu->temp_addr);
    struct vram_pointer v = decode_address(ppu->vram_addr);

    v.table_x = t.table_x;
    v.tile_x  = t.tile_x;

    ppu->vram_addr = encode_address(v);
}

static void copy_address_y(PPU *ppu)
{
    const struct vram_pointer t = decode_address(ppu->temp_addr);
    struct vram_pointer v = decode_address(ppu->vram_addr);

    v.table_y = t.table_y;
    v.tile_y  = t.tile_y;
    v.fine_y  = t.fine_y;

    ppu->vram_addr = encode_address(v);
}

static int address_increment(const PPU *ppu)
{
    return ppu->get_ctrl(CTRL_ADDR_INCREMENT) ? 32 : 1;
}

/* -------------------------------------------------------------------------- */
/* tile */

static uint8_t fetch_tile_id(const PPU *ppu)
{
    return ppu->read_byte(0x2000 | (ppu->vram_addr & 0x0FFF));
}

static uint8_t fetch_tile_attr(const PPU *ppu)
{
    const struct vram_pointer v = decode_address(ppu->vram_addr);
    const uint16_t attr_x = v.tile_x / 4;
    const uint16_t attr_y = v.tile_y / 4;
    const uint16_t offset = attr_y * 8 + attr_x;
    const uint16_t base = 0x2000 + 32 * 32 * v.table_x + 2 * 32 * 32 * v.table_y;
    const uint8_t attr = ppu->read_byte(base + 32 * 30 + offset);

    const uint8_t bit_x = v.tile_x % 4 > 1;
    const uint8_t bit_y = v.tile_y % 4 > 1;
    const uint8_t bit = (bit_y << 1) | bit_x;

    return (attr >> (bit * 2)) & 0x03;
}

static uint8_t fetch_tile_row(const PPU *ppu, uint8_t tile_id, uint8_t plane)
{
    const struct vram_pointer v = decode_address(ppu->vram_addr);
    const uint16_t base = ppu->get_ctrl(CTRL_PATTERN_BG) ? 0x1000 : 0x0000;
    const uint16_t addr = base + 16 * tile_id + plane + v.fine_y;

    return ppu->read_byte(addr);
}

static void clear_sprite_overflow(PPU *ppu)
{
    ppu->set_stat(STAT_SPRITE_OVERFLOW, 0);
}

static void load_next_tile(PPU *ppu, int cycle)
{
    /* after rendering is done in a scanline,
     * it is necesarry to shift bits for whole row before loading new row */
    if (cycle >= 321)
        ppu->tile_queue[2] = ppu->tile_queue[1];

    ppu->tile_queue[1] = ppu->tile_queue[0];
}

static void shift_word(uint8_t *left, uint8_t *right)
{
    *left <<= 1;
    *left |= (*right & 0x80) > 0;
    *right <<= 1;
}

static void shift_tile_data(PPU *ppu)
{
    shift_word(&ppu->tile_queue[2].lo, &ppu->tile_queue[1].lo);
    shift_word(&ppu->tile_queue[2].hi, &ppu->tile_queue[1].hi);
    shift_word(&ppu->tile_queue[2].palette_lo, &ppu->tile_queue[1].palette_lo);
    shift_word(&ppu->tile_queue[2].palette_hi, &ppu->tile_queue[1].palette_hi);
}

static void set_tile_palette(PatternRow *patt, uint8_t palette)
{
    patt->palette_lo = (palette & 0x01) ? 0xFF : 0x00;
    patt->palette_hi = (palette & 0x02) ? 0xFF : 0x00;
}

static void fetch_tile_data(PPU *ppu, int cycle)
{
    PatternRow *next = &ppu->tile_queue[0];

    switch (cycle % 8) {
    case 1:
        /* NT byte */
        next->id = fetch_tile_id(ppu);
        break;

    case 3:
        /* AT byte */
        set_tile_palette(next, fetch_tile_attr(ppu));
        break;

    case 5:
        /* Low BG tile byte */
        next->lo = fetch_tile_row(ppu, next->id, 0);
        break;

    case 7:
        /* High BG tile byte */
        next->hi = fetch_tile_row(ppu, next->id, 8);
        break;

    default:
        break;
    }
}

/* -------------------------------------------------------------------------- */
/* sprite */

static void clear_secondary_oam(PPU *ppu, int cycle)
{
    /* secondary oam crear occurs cycle 1 - 64 */
    if (cycle % 8 == 1) {
        const ObjectAttribute init = {0xFF, 0xFF, 0xFF, 0xFF};
        ppu->secondary_oam[cycle / 8] = init;
    }

    if (cycle == 1)
        ppu->sprite_count = 0;
}

static ObjectAttribute get_sprite(const PPU *ppu, int index)
{
    ObjectAttribute obj = {0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t attr = 0;

    if (index < 0 || index > 63)
        return obj;

    obj.y    = ppu->oam[4 * index + 0];
    obj.id   = ppu->oam[4 * index + 1];
    attr     = ppu->oam[4 * index + 2];
    obj.x    = ppu->oam[4 * index + 3];

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

static void evaluate_sprite(PPU *ppu, int cycle, int scanline)
{
    /* secondary oam crear occurs cycle 65 - 256 */
    /* index = 0 .. 191 */
    const int index = cycle - 65;
    const int sprite_height = 8;
    ObjectAttribute obj;

    if (index > 63)
        return;

    if (ppu->sprite_count > 8)
        return;

    obj = get_sprite(ppu, index);

    if (is_sprite_visible(obj, scanline, sprite_height)) {
        if (ppu->sprite_count < 8)
            ppu->secondary_oam[ppu->sprite_count] = obj;
        else
            ppu->set_sprite_overflow();

        ppu->sprite_count++;
    }
}

static uint8_t fetch_sprite_row(const PPU *ppu, uint8_t tile_id, int y, uint8_t plane)
{
    const uint16_t base = ppu->get_ctrl(CTRL_PATTERN_SPRITE) ? 0x1000 : 0x0000;
    const uint16_t addr = base + 16 * tile_id + plane + y;

    return ppu->read_byte(addr);
}

static uint8_t flip_pattern_row(uint8_t bits)
{
    uint8_t src = bits;
    uint8_t dst = 0x00;
    int i;

    for (i = 0; i < 8; i++) {
        dst = (dst << 1) | (src & 0x01);
        src >>= 1;
    }

    return dst;
}

static void fetch_sprite_data(PPU *ppu, int cycle, int scanline)
{
    /* fetch sprite data occurs cycle 257 - 320 */
    /* index = (0 .. 63) / 8 => 0 .. 7 */
    const int index = (cycle - 257) / 8;
    const int is_visible = index < ppu->sprite_count;
    const int sprite_id = ppu->secondary_oam[index].id;
    int sprite_y = scanline - ppu->secondary_oam[index].y;
    PatternRow *patt = &ppu->rendering_sprite[index];
    const ObjectAttribute obj = ppu->rendering_oam[index];

    switch (cycle % 8) {
    case 3:
        /* Attribute and X position */
        ppu->rendering_oam[index] = ppu->secondary_oam[index];

        if (is_visible)
            set_tile_palette(patt, ppu->rendering_oam[index].palette);
        else
            set_tile_palette(patt, 0x00);
        break;

    case 5:
        /* Low sprite byte */
        if (obj.flipped_v)
            sprite_y = 7 - sprite_y;

        if (is_visible)
            patt->lo = fetch_sprite_row(ppu, sprite_id, sprite_y, 0);
        else
            patt->lo = 0x00;

        if (obj.flipped_h)
            patt->lo = flip_pattern_row(patt->lo);
        break;

    case 7:
        /* High sprite byte */
        if (obj.flipped_v)
            sprite_y = 7 - sprite_y;

        if (is_visible)
            patt->hi = fetch_sprite_row(ppu, sprite_id, sprite_y, 8);
        else
            patt->hi = 0x00;

        if (obj.flipped_h)
            patt->hi = flip_pattern_row(patt->hi);
        break;

    default:
        break;
    }
}

static void shift_sprite_data(PPU *ppu)
{
    int i;

    for (i = 0; i < 8; i++) {
        ObjectAttribute *obj = &ppu->rendering_oam[i];
        PatternRow *patt = &ppu->rendering_sprite[i];

        if (obj->x > 0) {
            obj->x--;
        } else {
            patt->lo <<= 1;
            patt->hi <<= 1;
            patt->palette_lo <<= 1;
            patt->palette_hi <<= 1;
        }
    }
}

/* -------------------------------------------------------------------------- */
/* rendering */

struct pixel {
    uint8_t value;
    uint8_t palette;
    uint8_t priority;
    uint8_t sprite_zero;
};

static const struct pixel BACKDROP = {0};

static struct pixel get_pixel(PatternRow patt, uint8_t fine_x)
{
    struct pixel pix = {0};

    const uint8_t mask = 0x80 >> fine_x;
    const uint8_t hi = (patt.hi & mask) > 0;
    const uint8_t lo = (patt.lo & mask) > 0;
    const uint8_t val = (hi << 1) | lo;
    const uint8_t pal_lo = (patt.palette_lo & mask) > 0;
    const uint8_t pal_hi = (patt.palette_hi & mask) > 0;
    const uint8_t pal = (pal_hi << 1) | pal_lo;

    pix.value = val;
    pix.palette = pal;

    return pix;
}

static int is_sprite_zero(const PPU *ppu, ObjectAttribute obj)
{
    const int sprite_zero_id = ppu->oam[1];

    return obj.id == sprite_zero_id;
}

static struct pixel get_pixel_bg(const PPU *ppu)
{
    return get_pixel(ppu->tile_queue[2], ppu->fine_x);
}

static struct pixel get_pixel_fg(const PPU *ppu)
{
    int i;

    for (i = 0; i < 8; i++) {
        const ObjectAttribute obj = ppu->rendering_oam[i];

        if (obj.x == 0) {
            struct pixel pix = get_pixel(ppu->rendering_sprite[i], 0);

            pix.palette += 4;
            pix.priority = obj.priority;
            pix.sprite_zero = is_sprite_zero(ppu, obj);

            if (pix.value > 0)
                return pix;
        }
    }

    return BACKDROP;
}

static struct pixel composite_pixels(struct pixel bg, struct pixel fg)
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

static int is_clipping_left(const PPU *ppu)
{
    return !(ppu->mask & MASK_SHOW_BG_LEFT) ||
           !(ppu->mask & MASK_SHOW_SPRITE_LEFT);
}

static int is_sprite_zero_hit(const PPU *ppu, struct pixel bg, struct pixel fg, int x)
{
    if (!fg.sprite_zero)
        return 0;

    if (!ppu->is_rendering_bg() || !ppu->is_rendering_sprite())
        return 0;

    if ((x >= 0 && x <= 7) && is_clipping_left(ppu))
        return 0;

    if (x == 255)
        return 0;

    if (fg.value == 0 || bg.value == 0)
        return 0;

    if (ppu->stat & STAT_SPRITE_ZERO_HIT)
        return 0;

    return 1;
}

static uint8_t fetch_palette_value(const PPU *ppu, uint8_t palette_id, uint8_t pixel_val)
{
    const uint16_t addr = 0x3F00 + 4 * palette_id + pixel_val;

    return ppu->read_byte(addr);
}

static struct Color lookup_pixel_color(const PPU *ppu, struct pixel pix)
{
    const uint8_t index = fetch_palette_value(ppu, pix.palette, pix.value);
    return get_color(index);
}

static void render_pixel(PPU *ppu, int x, int y)
{
    struct pixel bg = {0}, fg = {0}, final = {0};
    struct Color col = {0};

    if (ppu->is_rendering_bg())
        bg = get_pixel_bg(ppu);

    if (ppu->is_rendering_sprite() && y != 0)
        fg = get_pixel_fg(ppu);

    final = composite_pixels(bg, fg);
    col = lookup_pixel_color(ppu, final);

    if (!ppu->is_rendering_bg() && !ppu->is_rendering_sprite() &&
        ppu->vram_addr >= 0x3F00 && ppu->vram_addr <= 0x3FFF) {
        const uint8_t index = ppu->read_byte(ppu->vram_addr);
        col = get_color(index);
    }

    ppu->fbuf->SetColor(x, y, col);

    if (is_sprite_zero_hit(ppu, bg, fg, x))
        ppu->set_stat(STAT_SPRITE_ZERO_HIT, 1);
}

// --------------------------------------------------------------------------
// clock

void PPU::ClearNMI()
{
    nmi_generated = false;
}

bool PPU::IsSetNMI() const
{
    return nmi_generated;
}

bool PPU::IsFrameReady() const
{
    return cycle == 0 && scanline == 0;
}

void PPU::Clock()
{
    const bool is_rendering = is_rendering_bg() || is_rendering_sprite();

    if ((scanline >= 0 && scanline <= 239) || scanline == 261) {

        // fetch bg tile
        if ((cycle >= 1 && cycle <= 256) || (cycle >= 321 && cycle <= 337)) {

            if (cycle % 8 == 0)
                if (is_rendering)
                    increment_scroll_x(this);

            if (cycle % 8 == 1)
                load_next_tile(this, cycle);

            fetch_tile_data(this, cycle);
        }

        // inc vert(v)
        if (cycle == 256)
            if (is_rendering)
                increment_scroll_y(this);

        // hori(v) = hori(t)
        if (cycle == 257)
            if (is_rendering)
                copy_address_x(this);

        // vert(v) = vert(t)
        if ((cycle >= 280 && cycle <= 304) && scanline == 261)
            if (is_rendering)
                copy_address_y(this);

        // clear secondary oam
        if ((cycle >= 1 && cycle <= 64) && scanline != 261)
            clear_secondary_oam(this, cycle);

        // evaluate sprite for next scanline
        if ((cycle >= 65 && cycle <= 256) && scanline != 261)
            evaluate_sprite(this, cycle, scanline);

        // fetch sprite
        if (cycle >= 257 && cycle <= 320)
            fetch_sprite_data(this, cycle, scanline);
    }

    if (scanline == 241)
        if (cycle == 1)
            enter_vblank();

    if (scanline == 261) {
        if (cycle == 1) {
            leave_vblank();
            clear_sprite_overflow(this);
            set_stat(STAT_SPRITE_ZERO_HIT, 0);
        }
    }

    // render pixel
    if (scanline >= 0 && scanline <= 239) {
        if (cycle >= 1 && cycle <= 256) {
            render_pixel(this, cycle - 1, scanline);

            if (is_rendering_bg())
                shift_tile_data(this);

            if (is_rendering_sprite())
                shift_sprite_data(this);
        }
    }

    // advance cycle and scanline
    if (cycle == 339) {
        if (scanline == 261 && frame % 2 == 0) {
            // the end of the scanline for odd frames
            cycle = 0;
            scanline = 0;
            frame++;
        }
        else {
            cycle++;
        }
    }
    else if (cycle == 340) {
        if (scanline == 261) {
            cycle = 0;
            scanline = 0;
            frame++;
        }
        else {
            cycle = 0;
            scanline++;
        }
    }
    else {
        cycle++;
    }
}

void PPU::PowerUp()
{
    ctrl = 0x00;
    mask = 0x00;
    stat = 0xA0;
    oam_addr = 0x00;

    write_toggle = 0;

    temp_addr = 0x0000;
    vram_addr = 0x0000;
    read_buffer = 0x00;

    frame = 0;

    for (int i = 0; i < 256; i++)
        oam[i] = 0xFF;

    for (int i = 0; i < 32; i++)
        palette_ram[i] = 0xFF;

    for (int i = 0; i < 2048; i++)
        name_table[i] = 0xFF;
}

void PPU::Reset()
{
    ctrl = 0x00;
    mask = 0x00;
    stat &= 0x80;

    write_toggle = 0;

    temp_addr   = 0x0000;
    read_buffer = 0x00;

    frame = 0;

    for (int i = 0; i < 256; i++)
        oam[i] = 0xFF;
}

void PPU::WriteControl(uint8_t data)
{
    // Nametable x and y from control
    // t: ...GH.. ........ <- d: ......GH
    //    <used elsewhere> <- d: ABCDEF..
    ctrl = data;
    temp_addr = (temp_addr & 0xF3FF) | ((data & 0x03) << 10);
}

void PPU::WriteMask(uint8_t data)
{
    mask = data;
}

void PPU::WriteOamAddress(uint8_t addr)
{
    oam_addr = addr;
}

void PPU::WriteOamData(uint8_t data)
{
    oam[oam_addr] = data;
}

void PPU::WriteScroll(uint8_t data)
{
    if (write_toggle == 0) {
        // Coarse X and fine x
        // t: ....... ...ABCDE <- d: ABCDE...
        // x:              FGH <- d: .....FGH
        // w:                  <- 1
        fine_x = data & 0x07;
        temp_addr = (temp_addr & 0xFFE0) | (data >> 3);
        write_toggle = 1;
    } else {
        // Coarse Y and fine y
        // t: FGH..AB CDE..... <- d: ABCDEFGH
        // w:                  <- 0
        temp_addr = (temp_addr & 0xFC1F) | ((data >> 3) << 5);
        temp_addr = (temp_addr & 0x8FFF) | ((data & 0x07) << 12);
        write_toggle = 0;
    }
}

void PPU::WriteAddress(uint8_t addr)
{
    if (write_toggle == 0) {
        // High byte
        // t: .CDEFGH ........ <- d: ..CDEFGH
        //        <unused>     <- d: AB......
        // t: Z...... ........ <- 0 (bit Z is cleared)
        // w:                  <- 1
        temp_addr = (temp_addr & 0xC0FF) | ((addr & 0x3F) << 8);
        temp_addr &= 0xBFFF;
        write_toggle = 1;
    } else {
        // Low byte
        // t: ....... ABCDEFGH <- d: ABCDEFGH
        // v: <...all bits...> <- t: <...all bits...>
        // w:                  <- 0
        temp_addr = (temp_addr & 0xFF00) | addr;
        vram_addr = temp_addr;
        write_toggle = 0;
    }
}

void PPU::WriteData(uint8_t data)
{
    write_byte(vram_addr, data);

    vram_addr += address_increment(this);
}

uint8_t PPU::ReadStatus()
{
    const uint8_t data = stat;

    set_stat(STAT_VERTICAL_BLANK, 0);
    write_toggle = 0;

    return data;
}

uint8_t PPU::ReadOamData() const
{
    return oam[oam_addr];
}

uint8_t PPU::ReadData()
{
    const uint16_t addr = vram_addr;
    uint8_t data = read_buffer;

    read_buffer = read_byte(addr);

    if (addr >= 0x3F00 && addr <= 0x3FFF)
        data = read_buffer;

    vram_addr += address_increment(this);

    return data;
}

uint8_t PPU::PeekStatus() const
{
    return stat;
}

void PPU::WriteDmaSprite(uint8_t addr, uint8_t data)
{
    oam[addr] = data;
}

ObjectAttribute PPU::ReadOam(int index) const
{
    return get_sprite(this, index);
}

} // namespace
