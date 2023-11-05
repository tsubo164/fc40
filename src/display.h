#ifndef DISPLAY_H
#define DISPLAY_H

#include <cstdint>
#include <string>

namespace nes {

class NES;

enum class MessageColor {
    White,
    Green,
    Red,
};

class Display {
public:
    Display(NES &nes);
    ~Display();

    int Open();

private:
    NES &nes_;
    uint64_t frame_ = 0;

    bool show_guide_ = false;
    bool show_patt_ = false;
    bool show_oam_ = false;

    std::string status_message_;
    int message_duration_ = 0;
    MessageColor message_color_ = MessageColor::White;
    uint64_t status_frame_ = 0;
    uint64_t channel_frame_ = 0;

    void init_video();

    void render() const;
    void render_overlay(double elapsed) const;
    void render_frame_rate(double elapsed) const;
    void render_channel_status() const;
    void render_status_message() const;
    void render_sprite_info() const;
    void render_ppu_info() const;

    void render_pattern_table() const;
    void render_oam_table() const;
    void render_grid(int width, int height) const;
    void render_sprite_box() const;

    void set_status_message(const std::string &message);
    void set_status_message(const std::string &message, MessageColor color, int duration);
    void toggle_channel_bits(uint8_t toggle_bits);
};

} // namespace

#endif // _H
