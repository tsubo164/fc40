#include <iostream>
#include <GLFW/glfw3.h>
#include "display.h"
#include "framebuffer.h"
#include "serialize.h"
#include "bitmap.h"
#include "debug.h"
#include "state.h"
#include "nes.h"
#include "ppu.h"

namespace nes {

const int MARGIN = 8;
const int SCREEN_MARGIN = 16;
const int VIDEO_SCALE = 2;
const int RESX = 256;
const int RESY = 240;
const float VIDEO_ASPECT = static_cast<float>(RESX + 2 * MARGIN) / (RESY + 2 * MARGIN);

struct KeyState {
    GLFWwindow *window = nullptr;
    bool pressed[GLFW_KEY_LAST] = {0};
    bool IsPressed(int key);
};

static const GLuint main_screen = 0;
static GLuint pattern_table_id = 0;
static GLuint oam_table_id = 0;

struct Coord {
    float x = 0.f, y = 0.f;
};

static float video_w = 0.f;
static float video_h = 0.f;

static int screen_w = 0;
static int screen_h = 0;
static float screen_aspect = 1.0f;
// how many display screen pixels per NES video res
static float screen_per_video = 1.0;

static float video_margin_x = 8.0f;
static float video_margin_y = 8.0f;

static int cursor_video_x = 0;
static int cursor_video_y = 0;
static int focused_oam_index = -1;

static int font_w = 8;
static int font_h = 8;

static void transfer_texture(const FrameBuffer &fb);
static void resize(GLFWwindow *window, int width, int height);
static void cursor_position(GLFWwindow *window, double xpos, double ypos);

static Coord screen_to_video(float screen_x, float screen_y);
static Coord video_to_screen(float video_x, float video_y);

enum TextDecoration {
    TEXT_FLAT,
    TEXT_OUTLINE,
};

static void draw_text(const std::string &text, int x, int y);
static void draw_text(const std::string &text, int x, int y, TextDecoration deco);

Display::Display(NES &nes) : nes_(nes)
{
    // Init bitmap fonts
    InitBitmapFont();
    GetBitmapFontSize(font_w, font_h);
}

Display::~Display()
{
}

int Display::Open()
{
    // MacOS Retina display has twice res
    const int WINX = RESX * VIDEO_SCALE + 2 * SCREEN_MARGIN;
    const int WINY = RESY * VIDEO_SCALE + 2 * SCREEN_MARGIN;
    KeyState key;

    // Initialize the library
    if (!glfwInit())
        return -1;

    // Create a windowed mode window and its OpenGL context
    GLFWwindow *window = glfwCreateWindow(WINX, WINY, "Famicom Emulator", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeCallback(window, resize);
    glfwSetCursorPosCallback(window, cursor_position);

    init_video();
    resize(window, WINX, WINY);
    key.window = window;

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window)) {
        glfwSetTime(0.0);

        // Update framebuffer
        nes_.UpdateFrame();

        // Render here
        render();

        // Render overlay
        render_overlay(glfwGetTime());

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();

        // Inputs
        uint8_t input = 0x00;
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
            input |= 1 << 7; // A
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
            input |= 1 << 6; // B
        if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
            input |= 1 << 5; // select
        if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
            input |= 1 << 4; // start
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            input |= 1 << 3; // up
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            input |= 1 << 2; // down
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            input |= 1 << 1; // left
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            input |= 1 << 0; // right
        nes_.InputController(0, input);

        // Keys display
        if (key.IsPressed(GLFW_KEY_G)) {
            show_guide_ = !show_guide_;
        }
        else if (key.IsPressed(GLFW_KEY_P)) {
            show_patt_ = !show_patt_;
        }
        else if (key.IsPressed(GLFW_KEY_O)) {
            show_oam_ = !show_oam_;
        }
        // Keys sound channels
        else if (key.IsPressed(GLFW_KEY_1)) {
            toggle_channel_bits(0x01);
        }
        else if (key.IsPressed(GLFW_KEY_2)) {
            toggle_channel_bits(0x02);
        }
        else if (key.IsPressed(GLFW_KEY_3)) {
            toggle_channel_bits(0x04);
        }
        else if (key.IsPressed(GLFW_KEY_4)) {
            toggle_channel_bits(0x08);
        }
        else if (key.IsPressed(GLFW_KEY_5)) {
            toggle_channel_bits(0x10);
        }
        else if (key.IsPressed(GLFW_KEY_GRAVE_ACCENT)) {
            toggle_channel_bits(0x1F);
        }
        // Keys debug
        else if (key.IsPressed(GLFW_KEY_SPACE)) {
            if (nes_.IsRunning()) {
                nes_.Pause();
                set_status_message("Emulator paused", MessageColor::Red, -1);
            }
            else {
                nes_.Run();
                set_status_message("Emulator resumed", MessageColor::Green, 4 * 60);
            }
        }
        else if (key.IsPressed(GLFW_KEY_7)) {
            if (!nes_.IsRunning())
                nes_.StepTo(NEXT_INSTRUCTION);
        }
        else if (key.IsPressed(GLFW_KEY_8)) {
            if (!nes_.IsRunning())
                nes_.StepTo(NEXT_SCANLINE1);
        }
        else if (key.IsPressed(GLFW_KEY_9)) {
            if (!nes_.IsRunning())
                nes_.StepTo(NEXT_SCANLINE8);
        }
        else if (key.IsPressed(GLFW_KEY_0)) {
            if (!nes_.IsRunning())
                nes_.StepTo(NEXT_FRAME);
        }
        else if (key.IsPressed(GLFW_KEY_F1)) {
            const Cartridge *cart = nes_.GetCartridge();
            const std::string stat_filename = cart->GetFileName() + ".stat";
            const bool ok = SaveState(nes_, stat_filename);
            if (ok)
                set_status_message(stat_filename + ": saved successfully");
            else
                set_status_message(stat_filename + ": failed to save");
        }
        else if (key.IsPressed(GLFW_KEY_F2)) {
            const Cartridge *cart = nes_.GetCartridge();
            const std::string stat_filename = cart->GetFileName() + ".stat";
            const bool ok = LoadState(nes_, stat_filename);
            if (ok)
                set_status_message(stat_filename + ": loaded successfully");
            else
                set_status_message(stat_filename + ": failed to load");
        }
        // Keys reset
        else if (key.IsPressed(GLFW_KEY_R)) {
            nes_.PushResetButton();
        }
        else if (key.IsPressed(GLFW_KEY_Q)) {
            break;
        }

        // Gamepad
        GLFWgamepadstate state;
        if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state)) {
            uint8_t input = 0x00;

            if (state.buttons[GLFW_GAMEPAD_BUTTON_B])
                input |= 1 << 7; // A
            if (state.buttons[GLFW_GAMEPAD_BUTTON_A])
                input |= 1 << 6; // B
            if (state.buttons[GLFW_GAMEPAD_BUTTON_BACK])
                input |= 1 << 5; // select
            if (state.buttons[GLFW_GAMEPAD_BUTTON_START])
                input |= 1 << 4; // start
            if (state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] == -1)
                input |= 1 << 3; // up
            if (state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] == 1)
                input |= 1 << 2; // down
            if (state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] == -1)
                input |= 1 << 1; // left
            if (state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] == 1)
                input |= 1 << 0; // right
            if (input)
                nes_.InputController(0, input);
        }

        frame_++;
    }

    glfwTerminate();
    return 0;
}

void Display::init_video()
{
    // main screen
    glBindTexture(GL_TEXTURE_2D, main_screen);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // pattern table
    glGenTextures(1, &pattern_table_id);
    glBindTexture(GL_TEXTURE_2D, pattern_table_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // oam table
    glGenTextures(1, &oam_table_id);
    glBindTexture(GL_TEXTURE_2D, oam_table_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, main_screen);
    // bg color
    constexpr float bg = .25;
    glClearColor(bg, bg, bg, 0);
}

void Display::render() const
{
    const int W = nes_.fbuf.Width();
    const int H = nes_.fbuf.Height();

    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, main_screen);
    transfer_texture(nes_.fbuf);

    glTranslatef(video_margin_x, video_margin_y, 0);
    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(0, 0);
        glTexCoord2f(1, 0); glVertex2f(W, 0);
        glTexCoord2f(1, 1); glVertex2f(W, H);
        glTexCoord2f(0, 1); glVertex2f(0, H);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    if (show_guide_) {
        render_grid(W, H);
        render_sprite_box();
    }

    if (show_patt_) {
        LoadPatternTable(nes_.patt, nes_.GetCartridge());
        render_pattern_table();
        render_palette_table();
    }

    if (show_oam_) {
        LoadOamTable(nes_.oam, nes_.ppu);
        render_oam_table();
    }

    if (!nes_.IsRunning()) {
        render_scanline_guide(W, H);
    }

    glFlush();
}

void Display::render_overlay(double elapsed) const
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();

    glLoadIdentity();
    glOrtho(0, screen_w, screen_h, 0, -1., 1.);
    {
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        render_frame_rate(elapsed);
        render_channel_status();
        render_status_message();
        render_sprite_info();

        if (!nes_.IsRunning()) {
            const int STEP_Y = 10;
            int next_y = 32;
            const int x = 16;

            next_y = render_cpu_info(x, next_y, STEP_Y);
            next_y = render_ppu_info(x, next_y + STEP_Y, STEP_Y);
            next_y = render_cart_info(x, next_y + STEP_Y, STEP_Y);
        }

        glPopMatrix();
    }
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}

void Display::render_frame_rate(double elapsed) const
{
    static char text[16] = {'\0'};
    static int count = 0;
    static double sum = 0;

    sum += elapsed;
    count++;

    if (count == 60) {
        const double fps = 1.0 / (sum / 60);
        sprintf(text, "FPS: %.2lf", fps);

        sum = 0;
        count = 0;
    }

    draw_text(text, 16, 8);
}

void Display::render_channel_status() const
{
    const bool all_channels_on = nes_.GetChannelEnable() == 0x1F;
    const bool timed_out = frame_ - channel_frame_ > 4 * 60;
    if (all_channels_on && timed_out)
        return;

    char text[32] = {'\0'};
    const uint8_t chan_bits = nes_.GetChannelEnable();
    const char c[] = "/ ";

    sprintf(text, "          %c %c %c %c %c",
            c[(chan_bits & 0x01) > 0],
            c[(chan_bits & 0x02) > 0],
            c[(chan_bits & 0x04) > 0],
            c[(chan_bits & 0x08) > 0],
            c[(chan_bits & 0x10) > 0]);

    glPushAttrib(GL_CURRENT_BIT);

    const int x = 8 * 16;
    const int y = 8;
    if (all_channels_on)
        glColor3f(0.f, 1.f, 0.f);
    else
        glColor3f(1.f, 1.f, 1.f);
    draw_text("Channels: 1 2 T N D", x, y);

    glColor3f(1.f, 0.f, 0.f);
    draw_text(text, x, y);

    glPopAttrib();
}

void Display::render_status_message() const
{
    if (frame_ - status_frame_ > message_duration_)
        return;

    const int x = 16;
    const int y = screen_h - 16 + 4;

    glPushAttrib(GL_CURRENT_BIT);

    switch (message_color_) {
    case MessageColor::Red:
        glColor3f(1.f, 0.25f, 0.25f);
        break;

    case MessageColor::Green:
        glColor3f(0.25f, 1.f, 0.25f);
        break;

    case MessageColor::White:
    default:
        glColor3f(1.f, 1.f, 1.f);
        break;
    }

    draw_text(status_message_, x, y);

    glPopAttrib();
}

void Display::render_sprite_info() const
{
    if (focused_oam_index < 0)
        return;

    // focused sprite
    const ObjectAttribute obj = nes_.ppu.ReadOam(focused_oam_index);
    const int x = obj.x + 8;
    const int y = obj.y + 1;

    const Coord screen = video_to_screen(x, y);

    int offset = 0;
    const int X = screen.x + 4;
    const int Y = screen.y;

    std::string text =
        std::string("oam index: ") + std::to_string(obj.oam_index);
    draw_text(text, X, Y + 8 * offset++, TEXT_OUTLINE);

    text = std::string("oam x: ") + std::to_string(obj.x);
    draw_text(text, X, Y + 8 * offset++, TEXT_OUTLINE);

    text = std::string("oam y: ") + std::to_string(obj.y);
    draw_text(text, X, Y + 8 * offset++, TEXT_OUTLINE);
}

int Display::render_cpu_info(int x, int y, int step_y) const
{
    const CpuStatus stat = nes_.cpu.GetStatus();
    char buf[16] = {'\0'};
    int offset = 0;

    glPushAttrib(GL_CURRENT_BIT);
    glColor3f(0.5f, 1.0f, 0.5f);
        std::string text = "CPU";
        draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);
    glPopAttrib();

    sprintf(buf, "$%04X", stat.pc);
    text = std::string("PC: ") + buf;
    draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);

    sprintf(buf, "$%02X", stat.a);
    text = std::string("A: ") + buf;
    draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);

    sprintf(buf, "$%02X", stat.x);
    text = std::string("X: ") + buf;
    draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);

    sprintf(buf, "$%02X", stat.y);
    text = std::string("Y: ") + buf;
    draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);

    sprintf(buf, "$%02X", stat.p);
    text = std::string("P: ") + buf;
    draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);

    sprintf(buf, "$%02X", stat.s);
    text = std::string("S: ") + buf;
    draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);

    return y + step_y * offset;
}

int Display::render_ppu_info(int x, int y, int step_y) const
{
    const PpuStatus stat = nes_.ppu.GetStatus();
    char buf[16] = {'\0'};
    int offset = 0;

    glPushAttrib(GL_CURRENT_BIT);
    glColor3f(0.5f, 1.0f, 0.5f);
        std::string text = "PPU";
        draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);
    glPopAttrib();

    text = std::string("cycle: ") + std::to_string(nes_.ppu.GetCycle());
    draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);

    text = std::string("scanlin: ") + std::to_string(nes_.ppu.GetScanline());
    draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);

    sprintf(buf, "$%02X", stat.ctrl);
    text = std::string("ctrl: ") + buf;
    draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);

    sprintf(buf, "$%02X", stat.mask);
    text = std::string("mask: ") + buf;
    draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);

    sprintf(buf, "$%02X", stat.stat);
    text = std::string("stat: ") + buf;
    draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);

    sprintf(buf, "$%02X", stat.fine_x);
    text = std::string("fine_x: ") + buf;
    draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);

    sprintf(buf, "$%04X", stat.vram_addr);
    text = std::string("vram_addr: ") + buf;
    draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);

    sprintf(buf, "$%04X", stat.temp_addr);
    text = std::string("temp_addr: ") + buf;
    draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);

    return y + step_y * offset;
}

int Display::render_cart_info(int x, int y, int step_y) const
{
    const Cartridge *cart = nes_.GetCartridge();
    char buf[16] = {'\0'};
    int offset = 0;

    CartridgeStatus stat;
    cart->GetCartridgeStatus(stat);

    glPushAttrib(GL_CURRENT_BIT);
    glColor3f(0.5f, 1.0f, 0.5f);
        std::string text = "Cartridge";
        draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);
    glPopAttrib();

    sprintf(buf, "%03d", stat.mapper_id);
    text = std::string("iNES Mapper: ") + buf;
    draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);

    sprintf(buf, "%luKB", stat.prg_size / 1024);
    text = std::string("PRG: ") + buf;
    draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);

    sprintf(buf, "%luKB", stat.chr_size / 1024);
    text = std::string("CHR: ") + buf;
    draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);

    sprintf(buf, "%d", stat.has_battery);
    text = std::string("Battery Present: ") + buf;
    draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);

    sprintf(buf, "%d", stat.mirroring);
    text = std::string("Mirroring: ") + buf;
    draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);

    offset++;

    sprintf(buf, "%d", stat.prg_bank_count);
    text = std::string("PRG bank count: ") + buf;
    draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);

    for (int i = 0; i < stat.prg_selected.size(); i++) {
        sprintf(buf, "[%d]: %d", i, stat.prg_selected[i]);
        text = std::string("  PRG") + buf;
        draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);
    }

    sprintf(buf, "%d", stat.chr_bank_count);
    text = std::string("CHR bank count: ") + buf;
    draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);

    for (int i = 0; i < stat.chr_selected.size(); i++) {
        sprintf(buf, "[%d]: %d", i, stat.chr_selected[i]);
        text = std::string("  CHR") + buf;
        draw_text(text, x, y + step_y * offset++, TEXT_OUTLINE);
    }

    return y + step_y * offset;
}

void Display::render_pattern_table() const
{
    const int W = nes_.patt.Width();
    const int H = nes_.patt.Height();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, pattern_table_id);
    transfer_texture(nes_.patt);

    glPushMatrix();
    glTranslatef(0, RESY / 2.0 - H / 2.0, 0);

    glBegin(GL_QUADS);
        glTexCoord2f(0, 1); glVertex2f(0, H);
        glTexCoord2f(1, 1); glVertex2f(W, H);
        glTexCoord2f(1, 0); glVertex2f(W, 0);
        glTexCoord2f(0, 0); glVertex2f(0, 0);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    glPushAttrib(GL_CURRENT_BIT);

    glBegin(GL_LINES);
    glColor3f(0, .5, .5);
    for (int i = 0; i <= W / 8; i++) {
        glVertex3f(0 + i * 8, 0, 0);
        glVertex3f(0 + i * 8, H, 0);
    }
    for (int i = 0; i <= H / 8; i++) {
        glVertex3f(0, 0 + i * 8, 0);
        glVertex3f(W, 0 + i * 8, 0);
    }
    glEnd();

    glColor3f(0, 1, 1);
    glBegin(GL_LINE_LOOP);
        glVertex2f(0, H);
        glVertex2f(W, H);
        glVertex2f(W, 0);
        glVertex2f(0, 0);
    glEnd();
    glBegin(GL_LINES);
        glVertex2f(W/2, 0);
        glVertex2f(W/2, H);
    glEnd();

    glPopAttrib();
    glPopMatrix();
}

void Display::render_palette_table() const
{
    const int H = nes_.patt.Height();
    const int SIZE = 8;

    glPushMatrix();
    glPushAttrib(GL_CURRENT_BIT);
    glTranslatef(0, RESY / 2.0 - H / 2.0 - SIZE, 0);

    for (int palette_id = 0; palette_id < 8; palette_id++) {
        // Palettes
        glBegin(GL_QUADS);
        for (int value = 0; value < 4; value++) {
            const float offsetx = (4 * palette_id + value) * SIZE;
            const Color color = nes_.ppu.GetPaletteColor(palette_id, value);

            glColor3f(color.r / 255., color.g / 255., color.b / 255.);
            glVertex2f(offsetx + 0,    SIZE);
            glVertex2f(offsetx + SIZE, SIZE);
            glVertex2f(offsetx + SIZE, 0);
            glVertex2f(offsetx + 0,    0);
        }
        glEnd();

        // Outlines
        glColor3f(1, 1, 1);
        glBegin(GL_LINE_LOOP);
            const float offsetx = 4 * palette_id * SIZE;
            glVertex2f(offsetx + 0,        SIZE);
            glVertex2f(offsetx + SIZE * 4, SIZE);
            glVertex2f(offsetx + SIZE * 4, 0);
            glVertex2f(offsetx + 0,        0);
        glEnd();
    }

    glPopAttrib();
    glPopMatrix();
}

void Display::render_oam_table() const
{
    const int W = nes_.oam.Width();
    const int H = nes_.oam.Height();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, oam_table_id);
    transfer_texture(nes_.oam);

    glPushMatrix();
    glTranslatef(RESX / 2.0 - W / 2.0, -8 * 10, 0);
    glTranslatef(0, RESY / 2.0 - H / 2.0, 0);

    glBegin(GL_QUADS);
        glTexCoord2f(0, 1); glVertex2f(0, H);
        glTexCoord2f(1, 1); glVertex2f(W, H);
        glTexCoord2f(1, 0); glVertex2f(W, 0);
        glTexCoord2f(0, 0); glVertex2f(0, 0);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    glPushAttrib(GL_CURRENT_BIT);

    glBegin(GL_LINES);
    glColor3f(.5, 0, .5);
    for (int i = 0; i <= W / 8; i++) {
        glVertex3f(0 + i * 8, 0, 0);
        glVertex3f(0 + i * 8, H, 0);
    }
    for (int i = 0; i <= H / 8; i++) {
        glVertex3f(0, 0 + i * 8, 0);
        glVertex3f(W, 0 + i * 8, 0);
    }
    glEnd();

    glColor3f(1, 0, 1);
    glBegin(GL_LINE_LOOP);
        glVertex2f(0, H);
        glVertex2f(W, H);
        glVertex2f(W, 0);
        glVertex2f(0, 0);
    glEnd();
    glPopAttrib();

    glPopMatrix();
}

void Display::render_grid(int width, int height) const
{
    const int W = width;
    const int H = height;

    glPushAttrib(GL_CURRENT_BIT);

    // grid
    glBegin(GL_LINES);
    for (int y = 0; y < H; y++) {
        const Scroll scroll = nes_.ppu.GetScroll(y);
        const int Y = (y + 8 * scroll.coarse_y + scroll.fine_y) % 240;

        if (Y % 32 == 0)
            glColor3f(0, 1, 0);
        else if (Y % 16 == 0)
            glColor3f(0, .5, 0);
        else
            glColor3f(0, .25, 0);

        if (Y % 8 == 0) {
            glVertex3f(W, y, 0);
            glVertex3f(0, y, 0);
        }

        for (int x = 0; x < W; x++) {
            const int X = x + 8 * scroll.coarse_x + scroll.fine_x;

            if (X % 32 == 0)
                glColor3f(0, 1, 0);
            else if (X % 16 == 0)
                glColor3f(0, .5, 0);
            else
                glColor3f(0, .25, 0);

            if (X % 8 == 0) {
                glVertex3f(x, y, 0);
                glVertex3f(x, y + 1, 0);
            }
        }
    }
    glEnd();

    // outer frame
    glBegin(GL_LINE_LOOP);
    glColor3f(0, 1, 0);
    glVertex3f(0, H, 0);
    glVertex3f(W, H, 0);
    glVertex3f(W, 0, 0);
    glVertex3f(0, 0, 0);
    glEnd();

    glPopAttrib();
}

void Display::render_sprite_box() const
{
    const int focus_x = cursor_video_x;
    const int focus_y = cursor_video_y;
    const int sprite_h = nes_.ppu.IsSprite8x16() ? 16 : 8;

    glPushAttrib(GL_CURRENT_BIT);

    focused_oam_index = -1;
    for (int i = 0; i < 64; i++) {
        // draw sprite zero last
        const int index = 63 - i;
        const ObjectAttribute obj = nes_.ppu.ReadOam(index);
        const int x = obj.x;
        const int y = obj.y + 1;

        if (obj.x <= focus_x && obj.x + 8 >= focus_x &&
            obj.y <= focus_y && obj.y + 8 >= focus_y) {
            focused_oam_index = index;
        }

        // sprite zero
        if (index == 0)
            glColor3f(1, 1, 0);
        else
            glColor3f(1, 0, 1);

        glBegin(GL_LINE_LOOP);
            glVertex3f(x + 0, y + 0, 0);
            glVertex3f(x + 8, y + 0, 0);
            glVertex3f(x + 8, y + sprite_h, 0);
            glVertex3f(x + 0, y + sprite_h, 0);
        glEnd();
    }

    // focused sprite
    if (focused_oam_index >= 0) {
        const ObjectAttribute obj = nes_.ppu.ReadOam(focused_oam_index);
        const int x = obj.x;
        const int y = obj.y + 1;
        glColor3f(1, 1, 1);

        glBegin(GL_LINE_LOOP);
            glVertex3f(x + 0, y + 0, 0);
            glVertex3f(x + 8, y + 0, 0);
            glVertex3f(x + 8, y + sprite_h, 0);
            glVertex3f(x + 0, y + sprite_h, 0);
        glEnd();
    }

    glPopAttrib();
}

void Display::render_scanline_guide(int width, int height) const
{
    const int W = width;
    const int H = height;

    const int cycle = nes_.ppu.GetCycle();
    const int scanline = nes_.ppu.GetScanline();

    glPushAttrib(GL_CURRENT_BIT);
    glColor3f(0, 1, 1);
    glBegin(GL_LINES);
        glVertex2f(cycle, 0);
        glVertex2f(cycle, H);
        glVertex2f(0, scanline);
        glVertex2f(W, scanline);
    glEnd();
    glPopAttrib();
}

void Display::set_status_message(const std::string &message)
{
    set_status_message(message, MessageColor::White, 4 * 60);
}

void Display::set_status_message(const std::string &message, MessageColor color, int duration)
{
    status_frame_ = frame_;
    status_message_ = message;
    message_color_ = color;
    message_duration_ = duration < 0 ? ~0u : duration;
}

void Display::toggle_channel_bits(uint8_t toggle_bits)
{
    uint8_t chan_bits = nes_.GetChannelEnable();

    if (toggle_bits == 0x1F) {
        if (chan_bits == 0x1F)
            chan_bits = 0x00;
        else
            chan_bits = 0x1F;
    }
    else {
        if (chan_bits & toggle_bits)
            chan_bits &= ~toggle_bits;
        else
            chan_bits |= toggle_bits;
    }

    nes_.SetChannelEnable(chan_bits);
    channel_frame_ = frame_;
}

static void transfer_texture(const FrameBuffer &fb)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, fb.Width(), fb.Height(),
            0, GL_RGB, GL_UNSIGNED_BYTE, fb.GetData());
}

static void resize(GLFWwindow *window, int width, int height)
{
    screen_w = width;
    screen_h = height;
    screen_aspect = static_cast<float>(screen_w) / screen_h;

    if (screen_aspect > VIDEO_ASPECT)
        screen_per_video = static_cast<float>(screen_h - 2 * SCREEN_MARGIN) / RESY;
    else
        screen_per_video = static_cast<float>(screen_w - 2 * SCREEN_MARGIN) / RESX;

    video_margin_x = (screen_w / screen_per_video - RESX) / 2;
    video_margin_y = (screen_h / screen_per_video - RESY) / 2;

    video_w = RESX + 2 * video_margin_x;
    video_h = RESY + 2 * video_margin_y;

    int fb_w = 0, fb_h = 0;
    // MacOS Retina display has different fb size than window size
    glfwGetFramebufferSize(window, &fb_w, &fb_h);
    glViewport(0, 0, fb_w, fb_h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0, video_w, video_h, 0, -1., 1.);
}

static void cursor_position(GLFWwindow *window, double xpos, double ypos)
{
    const Coord video = screen_to_video(xpos, ypos);

    cursor_video_x = video.x;
    cursor_video_y = video.y;
}

static Coord screen_to_video(float screen_x, float screen_y)
{
    Coord video;

    video.x = screen_x / screen_per_video - video_margin_x;
    video.y = screen_y / screen_per_video - video_margin_y;

    return video;
}

static Coord video_to_screen(float video_x, float video_y)
{
    Coord screen;

    screen.x = (video_x + video_margin_x) * screen_per_video;
    screen.y = (video_y + video_margin_y) * screen_per_video;

    return screen;
}

static void draw_text(const std::string &text, int x, int y)
{
    glRasterPos2i(x, y + 8);

    const int w = font_w;
    const int h = font_h;
    const int offset_x = w - 1;
    const int offset_y = 0;

    for (const auto ch: text) {
        const uint8_t *bits = GetBitmapChar(ch);

        glBitmap(w, h, 0, 0, offset_x, offset_y, bits);
    }
}

static void draw_text(const std::string &text, int x, int y, TextDecoration deco)
{
    if (deco == TEXT_OUTLINE) {
        glPushAttrib(GL_CURRENT_BIT);
        glColor3f(0., 0., 0.);
        draw_text(text, x - 1, y + 1);
        draw_text(text, x + 1, y + 1);
        draw_text(text, x - 1, y - 1);
        draw_text(text, x + 1, y - 1);

        draw_text(text, x, y + 1);
        draw_text(text, x + 1, y);
        draw_text(text, x - 1, y);
        draw_text(text, x, y - 1);

        glPopAttrib();
    }

    draw_text(text, x, y);
}

// Keys
bool KeyState::IsPressed(int key)
{
    const int state = glfwGetKey(window, key);

    if (state == GLFW_PRESS && !pressed[key]) {
        pressed[key] = true;
        return true;
    }
    else if (state == GLFW_RELEASE && pressed[key]) {
        pressed[key] = false;
        return false;
    }

    return false;
}

} // namespace
