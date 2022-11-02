#include <cstdio>
#include <GLFW/glfw3.h>
#include "display.h"
#include "framebuffer.h"
#include "nes.h"
#include "ppu.h"

namespace nes {

const int MARGIN = 8;
const int SCALE = 2;
const int RESX = 256;
const int RESY = 240;

static int show_guide = 0;
static int show_patt = 0;

struct KeyState {
    int g = 0, p = 0, r = 0;
};

static void transfer_texture(const FrameBuffer &fb);
static void resize(GLFWwindow *const window, int width, int height);
static void render_grid(int width, int height);
static void render_pattern_table(const FrameBuffer &patt);
static void render_sprite_box(const struct PPU &ppu, int width, int height);

static const GLuint main_screen = 0;
static GLuint pattern_table_id = 0;

int Display::Open() const
{
    // MacOS Retina display has twice res
    const int WIN_MARGIN = 2 * MARGIN;
    const int WINX = RESX * SCALE + 2 * WIN_MARGIN;
    const int WINY = RESY * SCALE + 2 * WIN_MARGIN;
    uint64_t f = 0;
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

    init_gl();
    resize(window, WINX, WINY);

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window)) {
        const double time = glfwGetTime();
        if (time > 1.) {
            static char title[64] = {'\0'};
            sprintf(title, "Famicom Emulator  FPS: %5.2f\n", f/time);
            glfwSetWindowTitle(window, title);
            glfwSetTime(0.);
            f = 0;
        }

        // Update framebuffer
        update_frame_func(nes);
        transfer_texture(nes->fbuf);

        // Render here
        render();

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
        input_controller_func(nes, 0, input);

        // Keys
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS && key.g == 0) {
            show_guide = !show_guide;
            key.g = 1;
        }
        else if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE && key.g == 1) {
            key.g = 0;
        }
        else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && key.p == 0) {
            show_patt = !show_patt;
            key.p = 1;
        }
        else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE && key.p == 1) {
            key.p = 0;
        }
        else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && key.r == 0) {
            push_reset_button(nes);
            key.r = 1;
        }
        else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE && key.r == 1) {
            key.r = 0;
        }
        else if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
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
                input_controller_func(nes, 0, input);
        }

        f++;
    }

    glfwTerminate();
    return 0;
}

void Display::init_gl() const
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

    transfer_texture(nes->patt);

    glBindTexture(GL_TEXTURE_2D, main_screen);

    // bg color
    constexpr float bg = .25;
    glClearColor(bg, bg, bg, 0);
}

void Display::render() const
{
    const int W = nes->fbuf.Width();
    const int H = nes->fbuf.Height();

    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(-W/2,  H/2);
        glTexCoord2f(1, 0); glVertex2f( W/2,  H/2);
        glTexCoord2f(1, 1); glVertex2f( W/2, -H/2);
        glTexCoord2f(0, 1); glVertex2f(-W/2, -H/2);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    if (show_guide) {
        render_grid(W, H);
        render_sprite_box(nes->ppu, W, H);
    }

    if (show_patt)
        render_pattern_table(nes->patt);

    glFlush();
}

static void transfer_texture(const FrameBuffer &fb)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, fb.Width(), fb.Height(),
            0, GL_RGB, GL_UNSIGNED_BYTE, fb.GetData());
}

static void resize(GLFWwindow *const window, int width, int height)
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

    glViewport(0, 0, fb_w, fb_h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(-win_w/2 - MARGIN, win_w/2 + MARGIN,
            -win_h/2 - MARGIN, win_h/2 + MARGIN, -1., 1.);
}

static void render_grid(int width, int height)
{
    const int W = width;
    const int H = height;

    glPushAttrib(GL_CURRENT_BIT);
    glBegin(GL_LINES);
    for (int i = 0; i <= W / 8; i++) {
        if (i % 4 == 0)
            glColor3f(0, 1, 0);
        else
        if (i % 2 == 0)
            glColor3f(0, .5, 0);
        else
            glColor3f(0, .25, 0);
        glVertex3f(-W/2 + i * 8,  H/2, 0);
        glVertex3f(-W/2 + i * 8, -H/2, 0);
    }
    for (int i = 0; i <= H / 8; i++) {
        if (i % 4 == 2 || i == 0)
            glColor3f(0, 1, 0);
        else
        if (i % 2 == 0)
            glColor3f(0, .5, 0);
        else
            glColor3f(0, .25, 0);
        glVertex3f( W/2, -H/2 + i * 8, 0);
        glVertex3f(-W/2, -H/2 + i * 8, 0);
    }
    glEnd();
    glPopAttrib();
}

static void render_pattern_table(const FrameBuffer &patt)
{
    const int W = patt.Width();
    const int H = patt.Height();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, pattern_table_id);
    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(-W/2,  H/2);
        glTexCoord2f(1, 0); glVertex2f( W/2,  H/2);
        glTexCoord2f(1, 1); glVertex2f( W/2, -H/2);
        glTexCoord2f(0, 1); glVertex2f(-W/2, -H/2);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    // switch texture back to default
    glBindTexture(GL_TEXTURE_2D, main_screen);

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

static void render_sprite_box(const struct PPU &ppu, int width, int height)
{
    glPushAttrib(GL_CURRENT_BIT);

    for (int i = 0; i < 64; i++) {
        // draw sprite zero last
        const int index = 63 - i;
        const struct object_attribute obj = read_oam(&ppu, index);
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
            glVertex3f(x + 8, y - 8, 0);
            glVertex3f(x + 0, y - 8, 0);
        glEnd();
    }

    glPopAttrib();
}

} // namespace
