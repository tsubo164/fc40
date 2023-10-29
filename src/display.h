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
    bool show_guide_ = 0;
    bool show_patt_ = 0;

    uint32_t pattern_table_id_ = 0;

    void init_video();

    void render() const;
    void render_overlay(double elapsed) const;
    void render_frame_rate(double elapsed) const;
    void render_channel_status() const;

    void render_pattern_table() const;
};

} // namespace

#endif // _H
