#include <stdio.h>
#include <stdlib.h>
#include "cartridge.h"
#include "mapper.h"

namespace nes {

uint8_t Cartridge::ReadProg(uint16_t addr) const
{
    return mapper->ReadProg(addr);
}

void Cartridge::WriteProg(uint16_t addr, uint8_t data)
{
    mapper->WriteProg(addr, data);
}

uint8_t Cartridge::ReadChar(uint16_t addr) const
{
    return mapper->ReadChar(addr);
}

void Cartridge::WriteChar(uint16_t addr, uint8_t data)
{
    mapper->WriteChar(addr, data);
}

static uint8_t *read_program(FILE *fp, size_t size)
{
    uint8_t *prog = (uint8_t*) calloc(size, sizeof(uint8_t));

    fread(prog, sizeof(uint8_t), size, fp);

    return prog;
}

static uint8_t *read_character(FILE *fp, size_t size)
{
    uint8_t *chr = (uint8_t*) calloc(size, sizeof(uint8_t));

    fread(chr, sizeof(uint8_t), size, fp);

    return chr;
}

Cartridge *open_cartridge(const char *filename)
{
    Cartridge *cart = NULL;
    FILE *fp = fopen(filename, "rb");
    char header[16] = {'\0'};

    fread(header, sizeof(char), 16, fp);

    if (header[0] != 'N' ||
        header[1] != 'E' ||
        header[2] != 'S' ||
        header[3] != 0x1a)
        return NULL;

    cart = (Cartridge*) malloc(sizeof(Cartridge));

    cart->prog_size = header[4] * 16 * 1024;
    cart->char_size = header[5] * 8 * 1024;
    cart->mirroring = header[6] & 0x01;
    cart->mapper_id = header[6] >> 4;
    cart->nbanks = header[4] == 1 ? 1 : 2;
    cart->prog_rom = read_program(fp, cart->prog_size);
    cart->char_rom = read_character(fp, cart->char_size);

    cart->mapper = open_mapper(cart->mapper_id,
            cart->prog_rom, cart->prog_size,
            cart->char_rom, cart->char_size);
    if (!cart->mapper)
        cart->mapper_supported = 0;
    else
        cart->mapper_supported = 1;

    fclose(fp);
    return cart;
}

void close_cartridge(Cartridge *cart)
{
    if (!cart)
        return;

    close_mapper(cart->mapper);
    cart->mapper = nullptr;

    free(cart->prog_rom);
    free(cart->char_rom);
    free(cart);
}

int is_vertical_mirroring(const Cartridge *cart)
{
    return cart->mirroring == 1;
}

} // namespace
