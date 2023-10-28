#ifndef DRAW_TEXT_H
#define DRAW_TEXT_H

#include <string>

namespace nes {

void InitBitmapFont();
void DrawText(const std::string &text, int x, int y);

} // namespace

#endif // _H
