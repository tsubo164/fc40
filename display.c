#include <stdio.h>
#include <GLFW/glfw3.h>

#include "display.h"
#include "framebuffer.h"
#include "ppu.h"

const int MARGIN = 8;
const int SCALE = 2;
const int RESX = 256;
const int RESY = 240;

static int show_guide = 0;
static int show_patt = 0;
static int press_g = 0;
static int press_p = 0;

static void transfer_texture(const struct framebuffer *fb);
static void resize(GLFWwindow *const window, int width, int height);
static void init_gl(const struct display *disp);
static void render(const struct display *disp);
static void render_grid(int width, int height);
static void render_pattern_table(const struct framebuffer *patt);
static void render_sprite_box(const struct PPU *ppu, int width, int height);

static const GLuint main_screen = 0;
static GLuint pattern_table = 0;

int open_display(const struct display *disp)
{
    /* MacOS Retina display has twice res */
    const int WIN_MARGIN = 2 * MARGIN;
    const int WINX = RESX * SCALE + 2 * WIN_MARGIN;
    const int WINY = RESY * SCALE + 2 * WIN_MARGIN;

    uint64_t f = 0;
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(WINX, WINY, "Famicom Emulator", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeCallback(window, resize);

    init_gl(disp);
    resize(window, WINX, WINY);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)) {
        const double time = glfwGetTime();
        if (time > 1.) {
            static char title[64] = {'\0'};
            sprintf(title, "Famicom Emulator  FPS: %5.2f\n", f/time);
            glfwSetWindowTitle(window, title);
            glfwSetTime(0.);
            f = 0;
        }

        /* Update framebuffer */
        disp->update_frame_func();
        transfer_texture(disp->fb);

        /* Render here */
        render(disp);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();

        {
            uint8_t input = 0x00;

            if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
                input |= 1 << 7; /* A */
            if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
                input |= 1 << 6; /* B */
            if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
                input |= 1 << 5; /* select */
            if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
                input |= 1 << 4; /* start */
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                input |= 1 << 3; /* up */
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                input |= 1 << 2; /* down */
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                input |= 1 << 1; /* left */
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                input |= 1 << 0; /* right */

            disp->input_controller_func(0, input);
        }

        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS && press_g == 0) {
            show_guide = !show_guide;
            press_g = 1;
        }
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE && press_g == 1) {
            press_g = 0;
        }
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && press_p == 0) {
            show_patt = !show_patt;
            press_p = 1;
        }
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE && press_p == 1) {
            press_p = 0;
        }

        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            break;

        f++;
    }

    glfwTerminate();
    return 0;
}

static void transfer_texture(const struct framebuffer *fb)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, fb->width, fb->height,
            0, GL_RGB, GL_UNSIGNED_BYTE, fb->data);
}

static void init_gl(const struct display *disp)
{
    const float bg = .25;

    /* main screen */
    glBindTexture(GL_TEXTURE_2D, main_screen);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    /* pattern table */
    glGenTextures(1, &pattern_table);
    glBindTexture(GL_TEXTURE_2D, pattern_table);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, disp->pattern_table->width, disp->pattern_table->height,
            0, GL_RGB, GL_UNSIGNED_BYTE, disp->pattern_table->data);

    glBindTexture(GL_TEXTURE_2D, main_screen);

    /* bg color */
    glClearColor(bg, bg, bg, 0);
}

static void render(const struct display *disp)
{
    const int W = disp->fb->width;
    const int H = disp->fb->height;

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
        render_sprite_box(disp->ppu, W, H);
    }

    if (show_patt)
        render_pattern_table(disp->pattern_table);

    glFlush();
}

static void resize(GLFWwindow *const window, int width, int height)
{
    float win_w, win_h, aspect;
    int fb_w, fb_h;

    /* MacOS Retina display has different fb size than window size */
    glfwGetFramebufferSize(window, &fb_w, &fb_h);
    aspect = (float) fb_w / fb_h;

    if (aspect > (float) RESX / RESY) {
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
    int i;

    glPushAttrib(GL_CURRENT_BIT);
    glBegin(GL_LINES);
    for (i = 0; i <= W / 8; i++) {
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
    for (i = 0; i <= H / 8; i++) {
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

static void render_pattern_table(const struct framebuffer *patt)
{
    const int W = patt->width;
    const int H = patt->height;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, pattern_table);
    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(-W/2,  H/2);
        glTexCoord2f(1, 0); glVertex2f( W/2,  H/2);
        glTexCoord2f(1, 1); glVertex2f( W/2, -H/2);
        glTexCoord2f(0, 1); glVertex2f(-W/2, -H/2);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    /* switch texture back to default */
    glBindTexture(GL_TEXTURE_2D, main_screen);

    glPushAttrib(GL_CURRENT_BIT);
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

static void render_sprite_box(const struct PPU *ppu, int width, int height)
{
    int i;

    glPushAttrib(GL_CURRENT_BIT);

    for (i = 0; i < 64; i++) {
        /* draw sprite zero last */
        const int index = 63 - i;
        const struct object_attribute obj = read_oam(ppu, index);
        const int x = obj.x - width / 2;
        const int y = height / 2 - obj.y - 1;

        /* sprite zero */
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
