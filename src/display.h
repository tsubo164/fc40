#ifndef DISPLAY_H
#define DISPLAY_H

#include <cstdint>

namespace nes {

struct NES;

class Display {
public:
    Display(struct NES &nes) : nes_(nes) {}
    ~Display() {}

    void (*update_frame_func)(struct NES *nes);
    void (*input_controller_func)(struct NES *nes, uint8_t id, uint8_t input);

    int Open();

private:
    struct NES &nes_;
    int show_guide_ = 0;
    int show_patt_ = 0;

    void render() const;
};

} // namespace

#endif // _H
