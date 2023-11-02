#include <iostream>
#include <GLFW/glfw3.h>
#include "display.h"
#include "framebuffer.h"
#include "draw_text.h"
#include "serialize.h"
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

static void transfer_texture(const FrameBuffer &fb);
static void resize(GLFWwindow *window, int width, int height);
static void cursor_position(GLFWwindow *window, double xpos, double ypos);

static const GLuint main_screen = 0;
static GLuint pattern_table_id = 0;

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

int Display::Open()
{
    // MacOS Retina display has twice res
    const int WINX = RESX * VIDEO_SCALE + 2 * SCREEN_MARGIN;
    const int WINY = RESY * VIDEO_SCALE + 2 * SCREEN_MARGIN;
    KeyState key;

    // Init bitmap fonts
    InitBitmapFont();

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
                nes_.Stop();
                set_status_message("Emulator paused", MessageColor::Red, -1);
            }
            else {
                nes_.Run();
                set_status_message("Emulator resumed", MessageColor::Green, 4 * 60);
            }
        }
        else if (key.IsPressed(GLFW_KEY_TAB)) {
            if (!nes_.IsRunning())
                nes_.Step();
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
        render_sprite_box(W, H);
    }

    if (show_patt_) {
        LoadPatternTable(nes_.patt, nes_.GetCartridge());
        render_pattern_table();
    }

    if (show_oam_) {
        LoadOamTable(nes_.oam, nes_.GetCartridge(), &nes_.ppu);
        render_oam_table();
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

    DrawText(text, 16, 16);
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
    const int y = 16;
    if (all_channels_on)
        glColor3f(0.f, 1.f, 0.f);
    else
        glColor3f(1.f, 1.f, 1.f);
    DrawText("Channels: 1 2 T N D", x, y);

    glColor3f(1.f, 0.f, 0.f);
    DrawText(text, x, y);

    glPopAttrib();
}

void Display::render_status_message() const
{
    if (frame_ - status_frame_ > message_duration_)
        return;

    const int x = 16;
    const int y = screen_h - 8;

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

    DrawText(status_message_, x, y);

    glPopAttrib();
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

void Display::render_oam_table() const
{
    const int W = nes_.oam.Width();
    const int H = nes_.oam.Height();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, pattern_table_id);
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
            glVertex3f( W/2, H/2 - y, 0);
            glVertex3f(-W/2, H/2 - y, 0);
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
                glVertex3f(-W/2 + x, H/2 - y, 0);
                glVertex3f(-W/2 + x, H/2 - y - 1, 0);
            }
        }
    }
    glEnd();

    // outer frame
    glBegin(GL_LINE_LOOP);
    glColor3f(0, 1, 0);
    glVertex3f(-W/2,  H/2, 0);
    glVertex3f( W/2,  H/2, 0);
    glVertex3f( W/2, -H/2, 0);
    glVertex3f(-W/2, -H/2, 0);
    glEnd();

    glPopAttrib();
}

void Display::render_sprite_box(int width, int height) const
{
    const int focus_x = cursor_video_x;
    const int focus_y = cursor_video_y;
    int focus_index = -1;

    const int sprite_h = nes_.ppu.IsSprite8x16() ? 16 : 8;

    glPushAttrib(GL_CURRENT_BIT);

    for (int i = 0; i < 64; i++) {
        // draw sprite zero last
        const int index = 63 - i;
        const ObjectAttribute obj = nes_.ppu.ReadOam(index);
        const int x = obj.x - width / 2;
        const int y = height / 2 - obj.y - 1;

        if (obj.x <= focus_x && obj.x + 8 >= focus_x &&
            obj.y <= focus_y && obj.y + 8 >= focus_y) {
            focus_index = index;
        }

        // sprite zero
        if (index == 0)
            glColor3f(1, 1, 0);
        else
            glColor3f(1, 0, 1);

        glBegin(GL_LINE_LOOP);
            glVertex3f(x + 0, y - 0, 0);
            glVertex3f(x + 8, y - 0, 0);
            glVertex3f(x + 8, y - sprite_h, 0);
            glVertex3f(x + 0, y - sprite_h, 0);
        glEnd();
    }

    // focused sprite
    if (focus_index >= 0) {
        const ObjectAttribute obj = nes_.ppu.ReadOam(focus_index);
        const int x = obj.x - width / 2;
        const int y = height / 2 - obj.y - 1;
        glColor3f(1, 1, 1);

        glBegin(GL_LINE_LOOP);
            glVertex3f(x + 0, y - 0, 0);
            glVertex3f(x + 8, y - 0, 0);
            glVertex3f(x + 8, y - sprite_h, 0);
            glVertex3f(x + 0, y - sprite_h, 0);
        glEnd();

        int offset = 0;
        const int X = x + 8 + 4;
        const int Y = obj.y - height / 2;
        std::string text =
            std::string("oam index: ") + std::to_string(obj.oam_index);
        glColor3f(0, 0, 0);
        DrawText(text, X + 1, Y + 8 * offset + 1);
        glColor3f(1, 1, 1);
        DrawText(text, X, Y + 8 * offset++);

        text = std::string("oam x: ") + std::to_string(obj.x);
        glColor3f(0, 0, 0);
        DrawText(text, X + 1, Y + 8 * offset + 1);
        glColor3f(1, 1, 1);
        DrawText(text, X, Y + 8 * offset++);

        text = std::string("oam y: ") + std::to_string(obj.y);
        glColor3f(0, 0, 0);
        DrawText(text, X + 1, Y + 8 * offset + 1);
        glColor3f(1, 1, 1);
        DrawText(text, X, Y + 8 * offset++);
    }

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
    const float aspect = static_cast<float>(screen_w) / screen_h;

    float video_x = 0;
    float video_y = 0;

    if (aspect > static_cast<float>(RESX + 2 * MARGIN) / (RESY + 2 * MARGIN)) {
        const float video_h = RESY + 2 * MARGIN;
        const float video_w = video_h * aspect;
        const float margin_y = MARGIN;
        const float margin_x = (video_w - RESX) / 2;
        video_y = ypos / screen_h * video_h - margin_y;
        video_x = xpos / screen_w * video_w - margin_x;

    }
    else {
        const float video_w = RESX + 2 * MARGIN;
        const float video_h = video_w / aspect;
        const float margin_x = MARGIN;
        const float margin_y = (video_h - RESY) / 2;
        video_y = ypos / screen_h * video_h - margin_y;
        video_x = xpos / screen_w * video_w - margin_x;
    }

    cursor_video_x = video_x;
    cursor_video_y = video_y;
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
