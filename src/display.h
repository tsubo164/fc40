#ifndef DISPLAY_H
#define DISPLAY_H

#include <cstdint>
#include <string>

namespace nes {

class NES;

class Display {
public:
    Display(NES &nes) : nes_(nes) {}
    ~Display() {}

    int Open();

private:
    NES &nes_;
    uint64_t frame_ = 0;

    bool show_guide_ = false;
    bool show_patt_ = false;
    bool show_oam_ = false;

    std::string status_message_;
    uint64_t status_frame_ = 0;
    uint64_t channel_frame_ = 0;

    void init_video();

    void render() const;
    void render_overlay(double elapsed) const;
    void render_frame_rate(double elapsed) const;
    void render_channel_status() const;
    void render_status_message() const;

    void render_pattern_table() const;
    void render_oam_table() const;
    void render_grid(int width, int height) const;
    void render_sprite_box(int width, int height) const;

    void set_status_message(const std::string &message);
    void toggle_channel_bits(uint8_t toggle_bits);
};

} // namespace

#endif // _H
