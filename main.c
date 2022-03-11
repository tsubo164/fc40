#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <GLFW/glfw3.h>

#include "framebuffer.h"
#include "cartridge.h"
#include "cpu.h"
#include "ppu.h"

struct CPU cpu = {0};

/* tmp */
int open_display(void);
static struct framebuffer *framebuffer;
static uint8_t *tmp_chr_rom;

int main(void)
{
    struct cartridge *cart = open_cartridge("./sample1.nes");

    if (!cart) {
        fprintf(stderr, "not a *.nes file\n");
        return -1;
    }

    printf("program size:   %ldKB\n", cart->prog_size / 1024);
    printf("character size: %ldKB\n", cart->char_size / 1024);

    /* run */
    cpu.prog = cart->prog_rom;
    cpu.prog_size = cart->prog_size;
    reset(&cpu);

    tmp_chr_rom = cart->char_rom;
    open_display();

    close_cartridge(cart);
    return 0;
}

const int MARGIN = 32;
const int SCALE = 2;
const int RESX = 256;
const int RESY = 240;
const int WINX = RESX * SCALE + MARGIN;
const int WINY = RESY * SCALE + MARGIN;

GLuint texture_id;

void transfer_texture(const uint8_t *pixels);
void init_gl(const uint8_t *pixels);
void render(void);
void render_grid(void);
void resize(GLFWwindow *const window, int w, int h);

int open_display(void)
{
    uint64_t f = 0;
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

    framebuffer = new_framebuffer(RESX, RESY);
    init_gl(framebuffer->data);
    resize(window, WINX, WINY);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)) {
        const double time = glfwGetTime();
        if (time > .1) {
            glfwSetTime(0.);
            printf("FPS: %8.3f\n", f/time);
            f = 0;
        }

        execute(&cpu);

        fill_bg_tile(framebuffer, tmp_chr_rom);
        transfer_texture(framebuffer->data);

        /* Render here */
        render();

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();

        f++;
    }

    free_framebuffer(framebuffer);
    glfwTerminate();
    return 0;
}

void transfer_texture(const uint8_t *pixels)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, RESX, RESY,
            0, GL_RGB, GL_UNSIGNED_BYTE, framebuffer->data);
}

void init_gl(const uint8_t *pixels)
{
    const float bg = .25;

    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, 3, RESX, RESY,
            0, GL_RGB, GL_UNSIGNED_BYTE, framebuffer->data);
    glClearColor(bg, bg, bg, 0);
}

void render(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glScalef(SCALE, SCALE, SCALE);

    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(-RESX/2,  RESY/2);
        glTexCoord2f(1, 0); glVertex2f( RESX/2,  RESY/2);
        glTexCoord2f(1, 1); glVertex2f( RESX/2, -RESY/2);
        glTexCoord2f(0, 1); glVertex2f(-RESX/2, -RESY/2);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    if (0)
        render_grid();

    glFlush();
}

void resize(GLFWwindow *const window, int width, int height)
{
    int win_w, win_h;
    int fb_w, fb_h;

    /* MaxOS Retina display has diffrent fb size than window size */
    glfwGetFramebufferSize(window, &fb_w, &fb_h);
    win_w = width;
    win_h = height;

    glViewport(0, 0, fb_w, fb_h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-win_w/2, win_w/2, -win_h/2, win_h/2, -1., 1.);
}

void render_grid(void)
{
    int i;

    glPushAttrib(GL_CURRENT_BIT);
    glColor3f(0, 1, 0);
    glBegin(GL_LINES);
    for (i = 0; i <= RESX / 8; i++) {
        glVertex3f(-RESX/2 + i * 8,  RESY/2, .1);
        glVertex3f(-RESX/2 + i * 8, -RESY/2, .1);
    }
    for (i = 0; i <= RESY / 8; i++) {
        glVertex3f( RESX/2, -RESY/2 + i * 8, .1);
        glVertex3f(-RESX/2, -RESY/2 + i * 8, .1);
    }
    glEnd();
    glPopAttrib();
}
