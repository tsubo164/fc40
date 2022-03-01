#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

uint8_t *read_program(FILE *fp, size_t size);
void print_program(uint8_t *prog, size_t size);
void read_character(FILE *fp, size_t size);

struct CPU {
    uint8_t *prog;
    size_t prog_size;

    uint16_t pc;
    struct status {
        char i;
    } stat;
} cpu = {0};

static uint8_t fetch(void);
void reset(void);
void run(void);

int main(void)
{
    FILE *fp = fopen("./sample1.nes", "rb");
    char header[16] = {'\0'};
    size_t prog_size = 0;
    size_t char_size = 0;
    uint8_t *prog = NULL;

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

    prog = read_program(fp, prog_size);
    read_character(fp, char_size);

    if (0)
        print_program(prog, prog_size);

    {
        cpu.prog = prog;
        cpu.prog_size = prog_size;

        reset();
        run();
    }

    fclose(fp);
    free(prog);
    return 0;
}

uint8_t *read_program(FILE *fp, size_t size)
{
    uint8_t *prog = calloc(size, sizeof(uint8_t));

    fread(prog, sizeof(uint8_t), size, fp);

    return prog;
}

void print_program(uint8_t *prog, size_t size)
{
    uint8_t *pc = prog;
    uint8_t *end = prog + size;
    uint16_t abs = 0;
    int is_immediate = 0;
    int is_absolute = 0;
    int is_zeropage = 0;

    while (pc < end) {
        const int addr = 0x8000 + (pc - prog);
        /* fetch */
        uint8_t data = *pc++;

        if (is_immediate) {
            printf("\t  #$%02X\n", data);
            is_immediate = 0;
            continue;
        }
        if (is_absolute == 2) {
            abs = data;
            is_absolute--;
            continue;
        }
        if (is_absolute == 1) {
            abs += data << 8;
            printf("\t  $%04X\n", abs);
            is_absolute--;
            continue;
        }
        if (is_zeropage) {
            const int curr = 0x8000 + (pc - prog);
            const int8_t offset = data;
            printf("\t  $%04X\n", curr + offset);
            is_zeropage = 0;
            continue;
        }

        printf("[%04X] ", addr);

        switch (data) {
        case 0x00 + 0x00:
            printf("BRK\n");
            break;

        case 0x40 + 0x0C:
            printf("JMP a");
            is_absolute = 2;
            break;

        case 0x60 + 0x18:
            printf("SEI\n");
            break;

        case 0x80 + 0x00:
            printf("NOP #i");
            is_immediate = 1;
            break;
        case 0x80 + 0x08:
            printf("DEY\n");
            break;
        case 0x80 + 0x0D:
            printf("STA a");
            is_absolute = 2;
            break;
        case 0x80 + 0x1A:
            printf("TXS\n");
            break;

        case 0xA0 + 0x00:
            printf("LDY #i");
            is_immediate = 1;
            break;
        case 0xA0 + 0x02:
            printf("LDX #i");
            is_immediate = 1;
            break;
        case 0xA0 + 0x09:
            printf("LDA #i");
            is_immediate = 1;
            break;
        case 0xA0 + 0x1D:
            printf("LDA (a,x)");
            is_absolute = 2;
            break;

        case 0xC0 + 0x10:
            printf("BNE *+d");
            is_zeropage = 1;
            break;

        case 0xE0 + 0x08:
            printf("INX\n");
            break;

        default:
            printf("0x%02X\n", data);
            break;
        }
    }
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

void read_character(FILE *fp, size_t size)
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

    free(chr);
}

static uint8_t read_byte(uint16_t addr)
{
    //return cpu->prog[addr - 0x8000];
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
    return read_byte(cpu.pc++);
}

static void jump(uint16_t addr)
{
    cpu.pc = addr;
}

void reset(void)
{
    uint16_t addr;

    addr = read_word(0xFFFC);
    jump(addr);
}

static void brk(void)
{
    /*
    printf("%s\n", __func__);
    */
}

static void sei(void)
{
    cpu.stat.i = 1;
    printf("%s\n", __func__);
}

void run(void)
{
    for (;;) {
        uint16_t addr = cpu.pc;
        uint8_t code = fetch();

        switch (code) {
        case 0x00 + 0x00:
            brk();
            break;

        case 0x60 + 0x18:
            printf("[%04X] ", addr);
            sei();
            break;

        default:
            break;
        }

        if (cpu.pc == cpu.prog_size + 0x8000 - 1)
            break;
    }
}
