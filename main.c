#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static uint16_t op_address = 0;

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

static uint16_t fetch_word(void)
{
    uint16_t lo, hi;

    lo = fetch();
    hi = fetch();

    return (hi << 8) + lo;
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

enum addressing_mode {
    IMP = 0, /* implied */
    ACC, /* A register */
    IMM, /* immediate */
    REL, /* immediate + pc */
    ABS, /* absolute */
    ABX, /* absolute + X */
    ABY  /* absolute + Y */
};

static void print_code(const char *op, int mode, int operand)
{
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
    uint16_t abs = cpu.pc;

    PRINT_CODE(mode, abs + imm);
}

static void brk(int mode)
{
    PRINT_CODE(mode, 0);
}

static void dey(int mode)
{
    PRINT_CODE(mode, 0);
}

static void inx(int mode)
{
    PRINT_CODE(mode, 0);
}

static void jmp(int mode)
{
    uint16_t abs = fetch_word();

    PRINT_CODE(mode, abs);
}

static void lda(int mode)
{
    uint8_t imm;
    uint16_t abs;

    switch (mode) {
    case IMM:
        imm = fetch();
        PRINT_CODE(mode, imm);
        break;

    case ABX:
        abs = fetch_word();
        PRINT_CODE(mode, abs);
        break;

    default:
        break;
    }
}

static void ldx(int mode)
{
    uint8_t imm = fetch();

    PRINT_CODE(mode, imm);
}

static void ldy(int mode)
{
    uint8_t imm = fetch();

    PRINT_CODE(mode, imm);
}

static void nop(int mode)
{
    uint8_t imm = fetch();

    PRINT_CODE(mode, imm);
}

static void sei(int mode)
{
    cpu.stat.i = 1;

    PRINT_CODE(mode, 0);
}

static void sta(int mode)
{
    uint16_t abs = fetch_word();

    PRINT_CODE(mode, abs);
}

static void txs(int mode)
{
    PRINT_CODE(mode, 0);
}

void run(void)
{
    while (cpu.pc) {
        const uint16_t addr = cpu.pc;
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
            printf("[%04X] ", op_address);
            printf("0x%02X\n", code);
            break;
        }
    }
}
