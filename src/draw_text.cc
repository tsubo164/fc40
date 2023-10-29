#include "draw_text.h"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <array>

namespace nes {

static const uint8_t fonts8x8[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ' '
    0x00, 0x18, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, // '!'
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x36, // '"'
    0x00, 0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, // '#'
    0x00, 0x18, 0x3E, 0x60, 0x3C, 0x06, 0x7C, 0x18, // '$'
    0x00, 0x33, 0x56, 0x6C, 0x1B, 0x35, 0x66, 0x00, // '%'
    0x00, 0xDE, 0x73, 0x3B, 0x6E, 0x16, 0x36, 0x1C, // '&'
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x18, 0x18, // '''
    0x00, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x18, 0x30, // '('
    0x00, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0C, // ')'
    0x00, 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, // '*'
    0x00, 0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, // '+'
    0x0C, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, // ','
    0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, // '-'
    0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, // '.'
    0x00, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, // '/'
    0x00, 0x3C, 0x66, 0x6E, 0x7E, 0x76, 0x66, 0x3C, // '0'
    0x00, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x1C, 0x18, // '1'
    0x00, 0x7E, 0x0C, 0x18, 0x30, 0x60, 0x66, 0x3C, // '2'
    0x00, 0x3C, 0x66, 0x60, 0x38, 0x60, 0x66, 0x3C, // '3'
    0x00, 0x30, 0x30, 0x7F, 0x33, 0x36, 0x3C, 0x38, // '4'
    0x00, 0x3C, 0x66, 0x60, 0x60, 0x3E, 0x06, 0x7E, // '5'
    0x00, 0x3C, 0x66, 0x66, 0x3E, 0x06, 0x0C, 0x38, // '6'
    0x00, 0x18, 0x18, 0x18, 0x30, 0x60, 0x60, 0x7E, // '7'
    0x00, 0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C, // '8'
    0x00, 0x1C, 0x30, 0x60, 0x7C, 0x66, 0x66, 0x3C, // '9'
    0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00, // ':'
    0x0C, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00, // ';'
    0x00, 0x00, 0x60, 0x18, 0x06, 0x18, 0x60, 0x00, // '<'
    0x00, 0x00, 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x00, // '='
    0x00, 0x00, 0x06, 0x18, 0x60, 0x18, 0x06, 0x00, // '>'
    0x00, 0x18, 0x00, 0x18, 0x30, 0x60, 0x66, 0x3C, // '?'
    0x00, 0x3C, 0x06, 0x7A, 0x5A, 0x5A, 0x66, 0x3C, // '@'
    0x00, 0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x3C, // 'A'
    0x00, 0x3E, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3E, // 'B'
    0x00, 0x78, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x78, // 'C'
    0x00, 0x1E, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1E, // 'D'
    0x00, 0x7E, 0x06, 0x06, 0x1E, 0x06, 0x06, 0x7E, // 'E'
    0x00, 0x06, 0x06, 0x06, 0x1E, 0x06, 0x06, 0x7E, // 'F'
    0x00, 0x7C, 0x66, 0x66, 0x76, 0x06, 0x66, 0x3C, // 'G'
    0x00, 0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, // 'H'
    0x00, 0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, // 'I'
    0x00, 0x3C, 0x66, 0x60, 0x60, 0x60, 0x60, 0x60, // 'J'
    0x00, 0x63, 0x33, 0x1B, 0x0F, 0x1B, 0x33, 0x63, // 'K'
    0x00, 0x7E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, // 'L'
    0x00, 0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, // 'M'
    0x00, 0x63, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x63, // 'N'
    0x00, 0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, // 'O'
    0x00, 0x06, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3E, // 'P'
    0x00, 0x7E, 0x3B, 0x33, 0x33, 0x33, 0x33, 0x1E, // 'Q'
    0x00, 0x66, 0x66, 0x36, 0x3E, 0x66, 0x66, 0x3E, // 'R'
    0x00, 0x3C, 0x66, 0x70, 0x3C, 0x0E, 0x66, 0x3C, // 'S'
    0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E, // 'T'
    0x00, 0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, // 'U'
    0x00, 0x18, 0x3C, 0x3C, 0x66, 0x66, 0x66, 0x66, // 'V'
    0x00, 0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x63, // 'W'
    0x00, 0xC3, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0xC3, // 'X'
    0x00, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x66, 0xC3, // 'Y'
    0x00, 0x7F, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x7F, // 'Z'
    0x00, 0x3C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3C, // '['
    0x00, 0xC0, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, // '\'
    0x00, 0x3C, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3C, // ']'
    0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x3C, 0x18, // '^'
    0x00, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // '_'
    0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x18, 0x18, // '`'
    0x00, 0x7C, 0x66, 0x7C, 0x60, 0x3C, 0x00, 0x00, // 'a'
    0x00, 0x3E, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x06, // 'b'
    0x00, 0x3C, 0x06, 0x06, 0x06, 0x3C, 0x00, 0x00, // 'c'
    0x00, 0x7C, 0x66, 0x66, 0x66, 0x7C, 0x60, 0x60, // 'd'
    0x00, 0x3C, 0x06, 0x7E, 0x66, 0x3C, 0x00, 0x00, // 'e'
    0x00, 0x0C, 0x0C, 0x0C, 0x0C, 0x3E, 0x0C, 0x38, // 'f'
    0x3C, 0x60, 0x7C, 0x66, 0x66, 0x7C, 0x00, 0x00, // 'g'
    0x00, 0x66, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x06, // 'h'
    0x00, 0x30, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, // 'i'
    0x1E, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x30, // 'j'
    0x00, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x06, 0x06, // 'k'
    0x00, 0x30, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, // 'l'
    0x00, 0x63, 0x63, 0x6B, 0x7F, 0x37, 0x00, 0x00, // 'm'
    0x00, 0x66, 0x66, 0x66, 0x66, 0x3E, 0x00, 0x00, // 'n'
    0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C, 0x00, 0x00, // 'o'
    0x06, 0x06, 0x3E, 0x66, 0x66, 0x3E, 0x00, 0x00, // 'p'
    0x60, 0x60, 0x7C, 0x66, 0x66, 0x7C, 0x00, 0x00, // 'q'
    0x00, 0x06, 0x06, 0x06, 0x66, 0x3E, 0x00, 0x00, // 'r'
    0x00, 0x3E, 0x60, 0x3C, 0x06, 0x3C, 0x00, 0x00, // 's'
    0x00, 0x38, 0x0C, 0x0C, 0x0C, 0x3E, 0x0C, 0x0C, // 't'
    0x00, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x00, 0x00, // 'u'
    0x00, 0x18, 0x3C, 0x66, 0x66, 0x66, 0x00, 0x00, // 'v'
    0x00, 0x36, 0x7F, 0x6B, 0x63, 0x63, 0x00, 0x00, // 'w'
    0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00, 0x00, // 'x'
    0x0C, 0x18, 0x3C, 0x66, 0x66, 0x66, 0x00, 0x00, // 'y'
    0x00, 0x7E, 0x0C, 0x18, 0x30, 0x7E, 0x00, 0x00, // 'z'
    0x00, 0x30, 0x18, 0x18, 0x0C, 0x18, 0x18, 0x30, // '{'
    0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, // '|'
    0x00, 0x0C, 0x18, 0x18, 0x30, 0x18, 0x18, 0x0C, // '}'
    0x00, 0x00, 0x00, 0x00, 0x00, 0x3B, 0x6E, 0x00, // '~'
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // DEL
};

static constexpr int FONT_BYTES = sizeof(fonts8x8) / sizeof(fonts8x8[0]);
static constexpr int FONT_SCALE = 2;
static constexpr int FONT_SCALE_2 = FONT_SCALE * FONT_SCALE;

static std::array<uint8_t, FONT_BYTES * FONT_SCALE_2> fonts;

bool sample_bit(const uint8_t *font8x8, int x, int y)
{
    if (x < 0 || x > 7)
        return false;
    if (y < 0 || y > 7)
        return false;

    const uint8_t row = font8x8[y];
    const bool bit = row & (0x1 << x);

    return bit;
}

void InitBitmapFont()
{
    for (int i = 0; i < FONT_BYTES / 8; i++) {
        const int index = 8 * i;

        uint8_t *dest = &fonts[index * FONT_SCALE_2];
        uint8_t buffer = 0;
        int count = 0;

        for (int y = 0; y < 8 * FONT_SCALE; y++) {
            const int Y = y / FONT_SCALE;

            for (int x = 0; x < 8 * FONT_SCALE; x++) {
                const int X = x / FONT_SCALE;
                const bool bit = sample_bit(&fonts8x8[index], X, Y);

                buffer = (buffer << 1) | bit;
                if (count == 7) {
                    *dest++ = buffer;
                    count = 0;
                }
                else {
                    count++;
                }
            }
        }
    }
}

void DrawText(const std::string &text, int x, int y)
{
    const int FONT_W = 8 * FONT_SCALE;
    const int FONT_H = 8 * FONT_SCALE;
    const int OFFSET_X = 7 * FONT_SCALE;
    const int OFFSET_Y = 0;

    glRasterPos2i(x, -8 - y);

    for (const auto ch: text)
        glBitmap(FONT_W, FONT_H, 0, 0, OFFSET_X, OFFSET_Y,
                &fonts[8 * FONT_SCALE_2 * (ch - ' ')]);
}

} // namespace