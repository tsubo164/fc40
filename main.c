#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <GLFW/glfw3.h>

#include "framebuffer.h"
#include "cartridge.h"
#include "ppu.h"

static uint16_t op_address = 0;
static int do_print_code = 0;

static void jump(uint16_t addr);

/* cpu */
struct status {
    char carry;
    char zero;
    char interrupt;
    char decimal;
    char brk;
    char reserved;
    char overflow;
    char negative;
};

struct registers {
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t s;
    struct status p;
    uint16_t pc;
};

struct CPU {
    uint8_t *prog;
    size_t prog_size;

    struct registers reg;
} cpu = {0};

static uint8_t fetch(void);
void reset(void);
void run(void);

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
    reset();
    run();

    tmp_chr_rom = cart->char_rom;
    open_display();

    close_cartridge(cart);
    return 0;
}

static void write_byte(uint16_t addr, uint8_t data)
{
    if (addr <= 0x07FF) {
        /* WRAM */
    }
    else if (addr <= 0x1FFF) {
        /* WRAM mirror */
    }
    else if (addr == 0x2000) {
    }
    else if (addr == 0x2006) {
        write_ppu_addr(data);
    }
    else if (addr == 0x2007) {
        write_ppu_data(data);
    }
    else if (addr <= 0x3FFF) {
        /* PPU registers mirror */
    }
}

static uint8_t read_byte(uint16_t addr)
{
    return cpu.prog[addr - 0x8000];
}

static uint16_t read_word(uint16_t addr)
{
    uint16_t lo, hi;

    lo = read_byte(addr);
    hi = read_byte(addr + 1);

    return (hi << 8) + lo;
}

static uint8_t fetch(void)
{
    return read_byte(cpu.reg.pc++);
}

static uint16_t fetch_word(void)
{
    uint16_t lo, hi;

    lo = fetch();
    hi = fetch();

    return (hi << 8) + lo;
}

static void jump(uint16_t addr)
{
    cpu.reg.pc = addr;
}

void reset(void)
{
    uint16_t addr;

    addr = read_word(0xFFFC);
    jump(addr);
}

enum addressing_mode {
    IMP = 0, /* implied */
    ACC, /* A register */
    IMM, /* immediate */
    REL, /* immediate + pc */
    ABS, /* absolute */
    ABX, /* absolute + X */
    ABY  /* absolute + Y */
};

static void print_code(const char *op, int mode, uint16_t operand)
{
    if (!do_print_code)
        return;

    printf("[%04X] %s", op_address, op);

    switch (mode) {
    case IMP:
        printf("\n");
        break;

    case IMM:
        printf(" #$%02X\n", operand);
        break;

    case REL:
    case ABS:
    case ABX:
        printf(" $%04X\n", operand);
        break;

    default:
        break;
    }
}

#define PRINT_CODE(mode, operand) print_code(__func__, mode, operand)

static void bne(int mode)
{
    int8_t imm = (int8_t) fetch();
    uint16_t abs = cpu.reg.pc;

    if (cpu.reg.p.zero == 0)
        jump(abs + imm);
    PRINT_CODE(mode, abs + imm);
}

static void brk(int mode)
{
    PRINT_CODE(mode, 0);
}

static void dey(int mode)
{
    cpu.reg.y--;
    cpu.reg.p.zero = (cpu.reg.y == 0);
    PRINT_CODE(mode, 0);
}

static void inx(int mode)
{
    cpu.reg.x++;
    PRINT_CODE(mode, 0);
}

static void jmp(int mode)
{
    static char tmp = 0;
    uint16_t abs = fetch_word();
    jump(abs);

    if (tmp == 0) {
        tmp++;
        PRINT_CODE(mode, abs);
    }
}

static void lda(int mode)
{
    uint8_t imm;
    uint16_t abs;

    switch (mode) {
    case IMM:
        imm = fetch();
        cpu.reg.a = imm;
        PRINT_CODE(mode, cpu.reg.a);
        break;

    case ABX:
        abs = fetch_word();
        cpu.reg.a = read_byte(abs + cpu.reg.x);
        PRINT_CODE(mode, abs + cpu.reg.x);
        break;

    default:
        break;
    }
}

static void ldx(int mode)
{
    uint8_t imm = fetch();
    cpu.reg.x = imm;

    PRINT_CODE(mode, imm);
}

static void ldy(int mode)
{
    uint8_t imm = fetch();
    cpu.reg.y = imm;

    PRINT_CODE(mode, imm);
}

static void nop(int mode)
{
    uint8_t imm = fetch();

    PRINT_CODE(mode, imm);
}

static void sei(int mode)
{
    cpu.reg.p.interrupt = 1;

    PRINT_CODE(mode, 0);
}

static void sta(int mode)
{
    uint16_t abs = fetch_word();
    write_byte(abs, cpu.reg.a);

    PRINT_CODE(mode, abs);
}

static void txs(int mode)
{
    cpu.reg.s = cpu.reg.x + 0x0100;
    PRINT_CODE(mode, 0);
}

void run(void)
{
    uint64_t cnt = 0;

    while (cpu.reg.pc) {
        const uint16_t addr = cpu.reg.pc;
        const uint8_t code = fetch();
        op_address = addr;

        switch (code) {
        case 0x00 + 0x00:
            brk(IMP);
            break;

        case 0x40 + 0x0C:
            jmp(ABS);
            break;

        case 0x60 + 0x18:
            sei(IMP);
            break;

        case 0x80 + 0x00:
            nop(IMM);
            break;
        case 0x80 + 0x08:
            dey(IMP);
            break;
        case 0x80 + 0x0D:
            sta(ABS);
            break;
        case 0x80 + 0x1A:
            txs(IMP);
            break;

        case 0xA0 + 0x00:
            ldy(IMM);
            break;
        case 0xA0 + 0x02:
            ldx(IMM);
            break;
        case 0xA0 + 0x09:
            lda(IMM);
            break;
        case 0xA0 + 0x1D:
            lda(ABX);
            break;

        case 0xC0 + 0x10:
            bne(REL);
            break;

        case 0xE0 + 0x08:
            inx(IMP);
            break;

        default:
            {
                char op_[8] = {'\0'};
                sprintf(op_, "0x%02X", code);
                print_code(op_, IMP, 0);
            }
            break;
        }

        if (cnt++ > 1024 * 1024) {
            printf("!!cnt reached: %llu\n", cnt);
            break;
        }
    }
}

const int MARGIN = 32;
const int SCALE = 2;
const int RESX = 256;
const int RESY = 240;
const int WINX = RESX * SCALE + MARGIN;
const int WINY = RESY * SCALE + MARGIN;

GLuint texture_id;

uint8_t *init_texture(void);
void init_gl(const uint8_t *pixels);
void render(void);
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
    fill_bg_tile(framebuffer, tmp_chr_rom);

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

uint8_t *init_texture(void)
{
    uint8_t *fbuf = calloc(RESX * RESY * 3, sizeof(uint8_t));

    return fbuf;
}

void init_gl(const uint8_t *pixels)
{
    const float bg = .25;

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, RESX, RESY,
            0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glClearColor(bg, bg, bg, 0);
}

void render(void)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glScalef(SCALE, SCALE, SCALE);

    glClear(GL_COLOR_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D , texture_id);

    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(-RESX/2,  RESY/2);
        glTexCoord2f(1, 0); glVertex2f( RESX/2,  RESY/2);
        glTexCoord2f(1, 1); glVertex2f( RESX/2, -RESY/2);
        glTexCoord2f(0, 1); glVertex2f(-RESX/2, -RESY/2);
    glEnd();
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

    if (0) {
        printf("(fb_w, fb_h) = (%d, %d)\n", fb_w, fb_h);
        printf("(win_w, win_h) = (%d, %d)\n", win_w, win_h);
        printf("(width, height) = (%d, %d)\n", width, height);
    }

    glViewport(0, 0, fb_w, fb_h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-win_w/2, win_w/2, -win_h/2, win_h/2, -1., 1.);
}
