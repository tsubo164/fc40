#include <stdio.h>
#include <GLFW/glfw3.h>
#include "display.h"
#include "framebuffer.h"

static void transfer_texture(const struct framebuffer *fb);
static void resize(GLFWwindow *const window, int width, int height);
static GLuint init_gl(const struct framebuffer *fb);
static void render(const struct framebuffer *fb, int scale);
static void render_grid(int w, int h);

int open_display(const struct framebuffer *fb, void (*update_frame_func)(void))
{
    const int MARGIN = 32;
    const int SCALE = 2;

    const int RESX = fb->width;
    const int RESY = fb->height;
    const int WINX = RESX * SCALE + MARGIN;
    const int WINY = RESY * SCALE + MARGIN;

    uint64_t f = 0;
    GLuint texture_id;
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

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
            printf("FPS: %8.3f\n", f/time);
            glfwSetTime(0.);
            f = 0;
        }

        /* Update framebuffer */
        update_frame_func();
        transfer_texture(fb);

        /* Render here */
        render(fb, SCALE);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();

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

static void render(const struct framebuffer *fb, int scale)
{
    const int W = fb->width;
    const int H = fb->height;

    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glScalef(scale, scale, scale);

    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(-W/2,  H/2);
        glTexCoord2f(1, 0); glVertex2f( W/2,  H/2);
        glTexCoord2f(1, 1); glVertex2f( W/2, -H/2);
        glTexCoord2f(0, 1); glVertex2f(-W/2, -H/2);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    if (0)
        render_grid(W, H);

    glFlush();
}

static void resize(GLFWwindow *const window, int width, int height)
{
    int win_w, win_h;
    int fb_w, fb_h;

    /* MaxOS Retina display has different fb size than window size */
    glfwGetFramebufferSize(window, &fb_w, &fb_h);
    win_w = width;
    win_h = height;

    glViewport(0, 0, fb_w, fb_h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-win_w/2, win_w/2, -win_h/2, win_h/2, -1., 1.);
}

static void render_grid(int w, int h)
{
    int i;

    glPushAttrib(GL_CURRENT_BIT);
    glColor3f(0, 1, 0);
    glBegin(GL_LINES);
    for (i = 0; i <= w / 8; i++) {
        glVertex3f(-w/2 + i * 8,  h/2, .1);
        glVertex3f(-w/2 + i * 8, -h/2, .1);
    }
    for (i = 0; i <= h / 8; i++) {
        glVertex3f( w/2, -h/2 + i * 8, .1);
        glVertex3f(-w/2, -h/2 + i * 8, .1);
    }
    glEnd();
    glPopAttrib();
}
