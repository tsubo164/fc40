#ifndef DISPLAY_H
#define DISPLAY_H

#include <cstdint>

namespace nes {

class NES;

class Display {
public:
    Display(NES &nes) : nes_(nes) {}
    ~Display() {}

    int Open();

private:
    NES &nes_;
    int show_guide_ = 0;
    int show_patt_ = 0;

    void render() const;
};

} // namespace

#endif // _H
