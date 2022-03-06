#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <GLFW/glfw3.h>

static uint16_t op_address = 0;
static int do_print_code = 0;

uint8_t *read_program(FILE *fp, size_t size);
uint8_t *read_character(FILE *fp, size_t size);
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

/* ppu */
static uint16_t ppu_addr;
static uint8_t ppu_data;
static uint8_t bg_pallet_table[16] = {0};
static uint8_t name_table_0[0x03C0] = {0};

void write_ppu_addr(uint8_t hi_or_lo);
void write_ppu_data(uint8_t data);

/* tmp */
static void print_bg_pallet_table(uint8_t *table);
static void print_name_table(uint8_t *table, uint16_t size, uint8_t *chr);
int open_display(void);

int main(void)
{
    FILE *fp = fopen("./sample1.nes", "rb");
    char header[16] = {'\0'};
    size_t prog_size = 0;
    size_t char_size = 0;
    uint8_t *prog_rom = NULL;
    uint8_t *char_rom = NULL;

    fread(header, sizeof(char), 16, fp);

    if (header[0] == 'N' &&
        header[1] == 'E' &&
        header[2] == 'S' &&
        header[3] == 0x1a) {
        ;
    } else {
        fprintf(stderr, "not a *.nes file\n");
        return -1;
    }

    prog_size = header[4] * 16 * 1024;
    char_size = header[5] * 8 * 1024;

    printf("header:          [%c%c%c]\n", header[0], header[1], header[2]);
    printf("program size:   %ldKB\n", prog_size / 1024);
    printf("character size: %ldKB\n", char_size / 1024);

    /* read */
    prog_rom = read_program(fp, prog_size);
    char_rom = read_character(fp, char_size);

    fclose(fp);

    /* run */
    cpu.prog = prog_rom;
    cpu.prog_size = prog_size;
    reset();
    run();

    printf("--------------------------------\n");
    print_bg_pallet_table(bg_pallet_table);
    printf("--------------------------------\n");
    print_name_table(name_table_0, sizeof(name_table_0), char_rom);

    open_display();

    free(prog_rom);
    free(char_rom);
    return 0;
}

uint8_t *read_program(FILE *fp, size_t size)
{
    uint8_t *prog = calloc(size, sizeof(uint8_t));

    fread(prog, sizeof(uint8_t), size, fp);

    return prog;
}

static void print_row(uint8_t r)
{
    uint8_t c[8] = {0};
    int mask = 1 << 7;
    int i;

    for (i = 0; i < 8; i++) {
        c[i] = (r & mask) > 0;
        printf("%d", c[i]);
        mask >>= 1;
    }

    printf("\n");
}

static void set_row(uint8_t r, uint8_t *dst, uint8_t bit)
{
    int mask = 1 << 7;
    int i;

    for (i = 0; i < 8; i++) {
        const uint8_t val = (r & mask) > 0;
        dst[i] += val << bit;
        mask >>= 1;
    }
}

uint8_t *read_character(FILE *fp, size_t size)
{
    uint8_t *chr = calloc(size, sizeof(uint8_t));
    int i, j;

    fread(chr, sizeof(uint8_t), size, fp);

    for (i = 0; i < size / 8; i++) {
        for (j = 0; j < 8; j++)
            if (0)
            print_row(chr[i * 8 + j]);
        if (0)
        printf("\n");
    }

    return chr;
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

/* ppu */
void write_ppu_addr(uint8_t hi_or_lo)
{
    static int is_high = 1;
    uint8_t data = hi_or_lo;

    ppu_addr = is_high ? data << 8 : ppu_addr + data;
    is_high = !is_high;
}

void write_ppu_data(uint8_t data)
{
    ppu_data = data;

    if (0x2000 <= ppu_addr && ppu_addr <= 0x23BF) {
        name_table_0[ppu_addr - 0x2000] = data;
        ppu_addr++;
    }
    if (0x3F00 <= ppu_addr && ppu_addr <= 0x3F0F) {
        bg_pallet_table[ppu_addr - 0x3F00] = data;
        ppu_addr++;
    }
}

/* tmp */
static void print_bg_pallet_table(uint8_t *table)
{
    printf("bg_pallet_table\n");
    for (int i = 0; i < 16; i++) {
        printf("0x%02X", table[i]);
        printf("%c", i%4==3 ? '\n' : ' ');
    }
}

static void print_name_table(uint8_t *table, uint16_t size, uint8_t *chr)
{
    int k;

    printf("name_table_0\n");

    for (k = 0; k < size; k++) {
        uint8_t data = table[k];

        if (data) {
            uint8_t obj[64] = {0};
            int i, j;

            i = 16 * data;
            for (j = 0; j < 8; j++)
                set_row(chr[i + j], &obj[j * 8], 0);

            i = 16 * data + 8;
            for (j = 0; j < 8; j++)
                set_row(chr[i + j], &obj[j * 8], 1);

            for (i = 0; i < 64; i++) {
                printf("%d", obj[i]);
                if (i % 8 == 7)
                    printf("\n");
            }
            printf("\n");
        }
    }
}

#define TEXX (512/8)
#define TEXY (480/8)
const int RESX = 512;
const int REXY = 480;

static GLubyte pixels[TEXY][TEXX][3];
GLuint texture_id;

void init_texture(void);
void init_gl(void);
void render(void);
void resize(GLFWwindow *const window, int w, int h);

int open_display(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(RESX, REXY, "Famicom Emulator", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeCallback(window, resize);

    init_texture();
    init_gl();
    resize(window, RESX, REXY);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)) {
        /* Render here */
        render();

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void init_texture(void)
{
    unsigned int i , j;

    for (i = 0 ; i < TEXY ; i++) {
        int r = (i * 0xFF) / TEXY;
        for (j = 0 ; j < TEXX ; j++) {
            pixels[i][j][0] = (GLubyte)r;
            pixels[i][j][1] = (GLubyte)(( j * 0xFF ) / TEXX);
            pixels[i][j][2] = (GLubyte)~r;
        }
    }
}

void init_gl(void)
{
    const float bg = .25;

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, TEXX, TEXY,
            0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glClearColor(bg, bg, bg, 0);
}

void render(void)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClear(GL_COLOR_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D , texture_id);

    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(-RESX/2, -REXY/2);
        glTexCoord2f(0, 1); glVertex2f(-RESX/2,  REXY/2);
        glTexCoord2f(1, 1); glVertex2f( RESX/2,  REXY/2);
        glTexCoord2f(1, 0); glVertex2f( RESX/2, -REXY/2);
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
