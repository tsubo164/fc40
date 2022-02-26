#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void read_program(FILE *fp, size_t size);
void read_character(FILE *fp, size_t size);

int main(void)
{
    FILE *fp = fopen("./sample1.nes", "rb");
    char header[16] = {'\0'};
    size_t prog_size = 0;
    size_t char_size = 0;

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

    read_program(fp, prog_size);
    read_character(fp, char_size);

    fclose(fp);
    return 0;
}

void read_program(FILE *fp, size_t size)
{
    uint8_t *prg = calloc(size, sizeof(uint8_t));
    int i;

    fread(prg, sizeof(uint8_t), size, fp);

    for (i = 0; i < 64; i++) {
    }

    free(prg);
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
            print_row(chr[i * 8 + j]);
        printf("\n");
    }

    free(chr);
}
