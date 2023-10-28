#include <iostream>
#include <fstream>
#include <chrono>
#include <GLFW/glfw3.h>
#include "display.h"
#include "framebuffer.h"
#include "draw_text.h"
#include "serialize.h"
#include "debug.h"
#include "nes.h"
#include "ppu.h"

namespace nes {

const int MARGIN = 8;
const int SCALE = 2;
const int RESX = 256;
const int RESY = 240;

struct KeyState {
    GLFWwindow *window = nullptr;
    bool pressed[GLFW_KEY_LAST] = {0};
    bool IsPressed(int key);
};

static void init_gl();
static void transfer_texture(const FrameBuffer &fb);
static void resize(GLFWwindow *window, int width, int height);
static void render_grid(const PPU &ppu, int width, int height);
static void render_pattern_table(const FrameBuffer &patt);
static void render_sprite_box(const PPU &ppu, int width, int height);
static void render_frame_rate(double elapsed);
static bool save_stat(NES &nes, const std::string &filename);
static bool load_stat(NES &nes, const std::string &filename);

// Audio channels
static uint8_t toggle_bits(uint8_t chan_bits, uint8_t toggle_bit);
static void print_channel_status(uint8_t chan_bits);

static const GLuint main_screen = 0;
static GLuint pattern_table_id = 0;

static int screen_w = 0;
static int screen_h = 0;

int Display::Open()
{
    // MacOS Retina display has twice res
    const int WIN_MARGIN = 2 * MARGIN;
    const int WINX = RESX * SCALE + 2 * WIN_MARGIN;
    const int WINY = RESY * SCALE + 2 * WIN_MARGIN;
    uint64_t f = 0;
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

    init_gl();
    resize(window, WINX, WINY);
    key.window = window;

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window)) {
        const auto start = std::chrono::high_resolution_clock::now();

        // Update framebuffer
        nes_.UpdateFrame();

        // Render here
        render();

        // Render FPS
        render_frame_rate(
                std::chrono::duration<double>(
                    std::chrono::high_resolution_clock::now() - start
                    ).count());

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
        // Keys sound channels
        else if (key.IsPressed(GLFW_KEY_1)) {
            uint8_t bits = nes_.GetChannelEnable();
            bits = toggle_bits(bits, 0x01);
            nes_.SetChannelEnable(bits);
            print_channel_status(bits);
        }
        else if (key.IsPressed(GLFW_KEY_2)) {
            uint8_t bits = nes_.GetChannelEnable();
            bits = toggle_bits(bits, 0x02);
            nes_.SetChannelEnable(bits);
            print_channel_status(bits);
        }
        else if (key.IsPressed(GLFW_KEY_3)) {
            uint8_t bits = nes_.GetChannelEnable();
            bits = toggle_bits(bits, 0x04);
            nes_.SetChannelEnable(bits);
            print_channel_status(bits);
        }
        else if (key.IsPressed(GLFW_KEY_4)) {
            uint8_t bits = nes_.GetChannelEnable();
            bits = toggle_bits(bits, 0x08);
            nes_.SetChannelEnable(bits);
            print_channel_status(bits);
        }
        else if (key.IsPressed(GLFW_KEY_5)) {
            uint8_t bits = nes_.GetChannelEnable();
            bits = toggle_bits(bits, 0x10);
            nes_.SetChannelEnable(bits);
            print_channel_status(bits);
        }
        else if (key.IsPressed(GLFW_KEY_6)) {
            uint8_t bits = nes_.GetChannelEnable();
            bits = (bits == 0x1F) ? 0x00 : 0x1F;
            nes_.SetChannelEnable(bits);
            print_channel_status(bits);
        }
        // Keys debug
        else if (key.IsPressed(GLFW_KEY_SPACE)) {
            if (nes_.IsRunning())
                nes_.Stop();
            else
                nes_.Run();
        }
        else if (key.IsPressed(GLFW_KEY_TAB)) {
            if (!nes_.IsRunning())
                nes_.Step();
        }
        else if (key.IsPressed(GLFW_KEY_F1)) {
            const Cartridge *cart = nes_.GetCartridge();
            const std::string stat_filename = cart->GetFileName() + ".stat";
            save_stat(nes_, stat_filename);
        }
        else if (key.IsPressed(GLFW_KEY_F2)) {
            const Cartridge *cart = nes_.GetCartridge();
            const std::string stat_filename = cart->GetFileName() + ".stat";
            load_stat(nes_, stat_filename);
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

        f++;
    }

    glfwTerminate();
    return 0;
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

    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(-W/2,  H/2);
        glTexCoord2f(1, 0); glVertex2f( W/2,  H/2);
        glTexCoord2f(1, 1); glVertex2f( W/2, -H/2);
        glTexCoord2f(0, 1); glVertex2f(-W/2, -H/2);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    if (show_guide_) {
        render_grid(nes_.ppu, W, H);
        render_sprite_box(nes_.ppu, W, H);
    }

    if (show_patt_) {
        LoadPatternTable(nes_.patt, nes_.GetCartridge());
        render_pattern_table(nes_.patt);
    }

    glFlush();
}

static void init_gl()
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

static void transfer_texture(const FrameBuffer &fb)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, fb.Width(), fb.Height(),
            0, GL_RGB, GL_UNSIGNED_BYTE, fb.GetData());
}

static void resize(GLFWwindow *window, int width, int height)
{
    float win_w = 0., win_h = 0.;
    int fb_w = 0, fb_h = 0;

    // MacOS Retina display has different fb size than window size
    glfwGetFramebufferSize(window, &fb_w, &fb_h);
    const float aspect = static_cast<float>(fb_w) / fb_h;

    if (aspect > static_cast<float>(RESX) / RESY) {
        win_w = RESY * aspect;
        win_h = RESY;
    } else {
        win_w = RESX;
        win_h = RESX / aspect;
    }
    screen_w = width;
    screen_h = height;

    glViewport(0, 0, fb_w, fb_h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(-win_w/2 - MARGIN, win_w/2 + MARGIN,
            -win_h/2 - MARGIN, win_h/2 + MARGIN, -1., 1.);
}

static void render_grid(const PPU &ppu, int width, int height)
{
    const int W = width;
    const int H = height;

    glPushAttrib(GL_CURRENT_BIT);

    // grid
    glBegin(GL_LINES);
    for (int y = 0; y < H; y++) {
        const Scroll scroll = ppu.GetScroll(y);
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

static void render_pattern_table(const FrameBuffer &patt)
{
    const int W = patt.Width();
    const int H = patt.Height();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, pattern_table_id);
    transfer_texture(patt);

    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(-W/2,  H/2);
        glTexCoord2f(1, 0); glVertex2f( W/2,  H/2);
        glTexCoord2f(1, 1); glVertex2f( W/2, -H/2);
        glTexCoord2f(0, 1); glVertex2f(-W/2, -H/2);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    glPushAttrib(GL_CURRENT_BIT);

    glBegin(GL_LINES);
    glColor3f(0, .5, .5);
    for (int i = 0; i <= W / 8; i++) {
        glVertex3f(-W/2 + i * 8,  H/2, 0);
        glVertex3f(-W/2 + i * 8, -H/2, 0);
    }
    for (int i = 0; i <= H / 8; i++) {
        glVertex3f( W/2, -H/2 + i * 8, 0);
        glVertex3f(-W/2, -H/2 + i * 8, 0);
    }
    glEnd();

    glColor3f(0, 1, 1);
    glBegin(GL_LINE_LOOP);
        glVertex2f(-W/2,  H/2);
        glVertex2f( W/2,  H/2);
        glVertex2f( W/2, -H/2);
        glVertex2f(-W/2, -H/2);
    glEnd();
    glBegin(GL_LINES);
        glVertex2f(0,  H/2);
        glVertex2f(0, -H/2);

    glEnd();
    glPopAttrib();
}

static void render_sprite_box(const PPU &ppu, int width, int height)
{
    const int sprite_h = ppu.IsSprite8x16() ? 16 : 8;

    glPushAttrib(GL_CURRENT_BIT);

    for (int i = 0; i < 64; i++) {
        // draw sprite zero last
        const int index = 63 - i;
        const ObjectAttribute obj = ppu.ReadOam(index);
        const int x = obj.x - width / 2;
        const int y = height / 2 - obj.y - 1;

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

    glPopAttrib();
}

static void render_frame_rate(double elapsed)
{
    static std::string text;
    static int count = 0;
    static double sum = 0;

    sum += elapsed;
    count++;

    if (count == 60) {
        const double fps = 1.0 / (sum / 60);
        static char buff[16] = {'\0'};
        sprintf(buff, "FPS: %.2lf", fps);
        text = buff;

        sum = 0;
        count = 0;
    }

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();

    glLoadIdentity();
    glOrtho(-8, screen_w, -screen_h, 8, -1., 1.);
    glColor3f(1.f, 1.f, 1.f);
    DrawText(text, 0, 0);

    glPopMatrix();
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

// Audio channels
static uint8_t toggle_bits(uint8_t chan_bits, uint8_t toggle_bit)
{
    if (chan_bits & toggle_bit)
        return chan_bits & ~toggle_bit;
    else
        return chan_bits | toggle_bit;
}

static void print_channel_status(uint8_t chan_bits)
{
    const char c[] = "-+";
    printf("====================\n");
    printf("Channels | 1 2 T N D\n");
    printf("ON/OFF   | %c %c %c %c %c\n",
            c[(chan_bits & 0x01) > 0],
            c[(chan_bits & 0x02) > 0],
            c[(chan_bits & 0x04) > 0],
            c[(chan_bits & 0x08) > 0],
            c[(chan_bits & 0x10) > 0]);
}

static bool save_stat(NES &nes, const std::string &filename)
{
    std::ofstream ofs(filename);
    if (!ofs)
        return false;

    Archive ar;

    Serialize(ar, "nes", &nes);
    ar.Write(ofs);

    std::cout << filename << ": saved successfully" << std::endl;
    return true;
}

static bool load_stat(NES &nes, const std::string &filename)
{
    std::ifstream ifs(filename);
    if (!ifs)
        return false;

    Archive ar;

    Serialize(ar, "nes", &nes);
    ar.Read(ifs);

    std::cout << filename << ": loaded successfully" << std::endl;
    return true;
}

} // namespace
