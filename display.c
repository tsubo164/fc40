#include <stdio.h>
#include <GLFW/glfw3.h>

#include "display.h"
#include "framebuffer.h"

const int MARGIN = 8;
const int SCALE = 2;
const int RESX = 256;
const int RESY = 240;

static int show_grid = 0;
static int key_press = 0;

static void transfer_texture(const struct framebuffer *fb);
static void resize(GLFWwindow *const window, int width, int height);
static GLuint init_gl(const struct framebuffer *fb);
static void render(const struct framebuffer *fb);
static void render_grid(int w, int h);

int open_display(const struct framebuffer *fb,
        void (*update_frame_func)(void),
        void (*input_controller_func)(uint8_t id, uint8_t input))
{
    /* MacOS Retina display has twice res */
    const int WIN_MARGIN = 2 * MARGIN;
    const int WINX = RESX * SCALE + 2 * WIN_MARGIN;
    const int WINY = RESY * SCALE + 2 * WIN_MARGIN;

    uint64_t f = 0;
    GLuint texture_id;
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

    texture_id = init_gl(fb);
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
        update_frame_func();
        transfer_texture(fb);

        /* Render here */
        render(fb);

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

            input_controller_func(0, input);
        }

        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS && key_press == 0) {
            show_grid = !show_grid;
            key_press = 1;
        }
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE && key_press == 1) {
            key_press = 0;
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

static GLuint init_gl(const struct framebuffer *fb)
{
    GLuint tex_id = 0;
    const float bg = .25;

    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glClearColor(bg, bg, bg, 0);

    return tex_id;
}

static void render(const struct framebuffer *fb)
{
    const int W = fb->width;
    const int H = fb->height;

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

    if (show_grid)
        render_grid(W, H);

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

static void render_grid(int w, int h)
{
    int i;

    glPushAttrib(GL_CURRENT_BIT);
    glBegin(GL_LINES);
    for (i = 0; i <= w / 8; i++) {
        if (i % 4 == 0)
            glColor3f(0, 1, 0);
        else
        if (i % 2 == 0)
            glColor3f(0, .5, 0);
        else
            glColor3f(0, .25, 0);
        glVertex3f(-w/2 + i * 8,  h/2, .1);
        glVertex3f(-w/2 + i * 8, -h/2, .1);
    }
    for (i = 0; i <= h / 8; i++) {
        if (i % 4 == 2 || i == 0)
            glColor3f(0, 1, 0);
        else
        if (i % 2 == 0)
            glColor3f(0, .5, 0);
        else
            glColor3f(0, .25, 0);
        glVertex3f( w/2, -h/2 + i * 8, .1);
        glVertex3f(-w/2, -h/2 + i * 8, .1);
    }
    glEnd();
    glPopAttrib();
}
