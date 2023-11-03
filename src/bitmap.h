#ifndef BITMAP_H
#define BITMAP_H

#include <cstdint>

namespace nes {

void InitBitmapFont();

void GetBitmapFontSize(int &width, int &height);
uint8_t *GetBitmapChar(int ch);

} // namespace

#endif // _H
