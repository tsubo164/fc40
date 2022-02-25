#include <stdio.h>

int main(void)
{
    FILE *fp = fopen("./sample1.nes", "rb");
    char header[16] = {'\0'};
    char prog_size = 0;
    char char_size = 0;

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

    prog_size = header[4];
    char_size = header[5];

    printf("header:          [%c%c%c]\n", header[0], header[1], header[2]);
    printf("program size:   %dKB\n", prog_size * 16);
    printf("character size: %dKB\n", char_size * 8);

    fclose(fp);
    return 0;
}
