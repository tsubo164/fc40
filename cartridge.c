#include <stdio.h>
#include <stdlib.h>
#include "cartridge.h"
#include "mapper.h"

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
    cart->mapper_id = header[6] >> 4;
    cart->nbanks = header[4] == 1 ? 1 : 2;
    cart->prog_rom = read_program(fp, cart->prog_size);
    cart->char_rom = read_character(fp, cart->char_size);

    const int err = open_mapper(&cart->mapper, cart->mapper_id,
            cart->prog_size, cart->char_size);

    if (err)
        cart->mapper_supported = 0;
    else
        cart->mapper_supported = 1;

    fclose(fp);
    return cart;
}

void close_cartridge(struct cartridge *cart)
{
    if (!cart)
        return;

    close_mapper(&cart->mapper);

    free(cart->prog_rom);
    free(cart->char_rom);
    free(cart);
}

uint8_t read_prog_rom(const struct cartridge *cart, uint16_t addr)
{
    const int32_t mapped = map_prog_addr(&cart->mapper, addr);

    if (mapped >= 0)
        return cart->prog_rom[mapped];
    else
        return 0;
}

uint8_t read_char_rom(const struct cartridge *cart, uint16_t addr)
{
    const int32_t mapped = map_char_addr(&cart->mapper, addr);

    if (mapped >= 0)
        return cart->char_rom[mapped];
    else
        return 0xFF;
}

void write_cartridge(const struct cartridge *cart, uint16_t addr, uint8_t data)
{
    write_mapper(&cart->mapper, addr, data);
}

int is_vertical_mirroring(const struct cartridge *cart)
{
    return cart->mirroring == 1;
}
