#include <stdio.h>
#include <stdlib.h>
#include "cartridge.h"

static uint8_t *read_program(FILE *fp, size_t size)
{
    uint8_t *prog = calloc(size, sizeof(uint8_t));

    fread(prog, sizeof(uint8_t), size, fp);

    return prog;
}

static uint8_t *read_character(FILE *fp, size_t size)
{
    uint8_t *chr = calloc(size, sizeof(uint8_t));

    fread(chr, sizeof(uint8_t), size, fp);

    return chr;
}

struct cartridge *open_cartridge(const char *filename)
{
    struct cartridge *cart = NULL;
    FILE *fp = fopen(filename, "rb");
    char header[16] = {'\0'};

    fread(header, sizeof(char), 16, fp);

    if (header[0] != 'N' ||
        header[1] != 'E' ||
        header[2] != 'S' ||
        header[3] != 0x1a)
        return NULL;

    cart = malloc(sizeof(struct cartridge));

    cart->prog_size = header[4] * 16 * 1024;
    cart->char_size = header[5] * 8 * 1024;
    cart->mirroring = header[6] & 0x01;
    cart->mapper = header[6] >> 4;
    cart->nbanks = header[4] == 1 ? 1 : 2;
    cart->prog_rom = read_program(fp, cart->prog_size);
    cart->char_rom = read_character(fp, cart->char_size);

    fclose(fp);
    return cart;
}

void close_cartridge(struct cartridge *cart)
{
    if (!cart)
        return;
    free(cart->prog_rom);
    free(cart->char_rom);
    free(cart);
}

uint8_t rom_read(const struct cartridge *cart, uint16_t addr)
{
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        uint16_t mapped;

        if (cart->mapper == 0)
            mapped = addr & (cart->nbanks == 1 ? 0x3FFF : 0x7FFF);
        else
            mapped = addr;

        return cart->prog_rom[mapped];
    }

    return 0;
}

uint8_t read_chr_rom(const struct cartridge *cart, uint16_t addr)
{
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        return cart->char_rom[addr];
    }

    return 0xFF;
}

int is_vertical_mirroring(const struct cartridge *cart)
{
    return cart->mirroring == 1;
}
