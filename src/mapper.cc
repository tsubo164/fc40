#include <stdio.h>
#include "mapper.h"
#include "mapper_002.h"

namespace nes {

static int prog_nbanks = 1;
static int char_nbanks = 1;

uint8_t Mapper::ReadProg(uint16_t addr) const
{
    return do_read_prog(addr);
}

void Mapper::WriteProg(uint16_t addr, uint8_t data)
{
    do_write_prog(addr, data);
}

uint8_t Mapper::ReadChar(uint16_t addr) const
{
    return do_read_char(addr);
}

void Mapper::WriteChar(uint16_t addr, uint8_t data)
{
    do_write_char(addr, data);
}

uint8_t Mapper::do_read_prog(uint16_t addr) const
{
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        const uint16_t a = addr & (prog_nbanks == 1 ? 0x3FFF : 0x7FFF);
        return prog_rom[a];
    }
    else {
        return 0;
    }
}

void Mapper::do_write_prog(uint16_t addr, uint8_t data)
{
}

uint8_t Mapper::do_read_char(uint16_t addr) const
{
    if (addr >= 0x0000 && addr <= 0x1FFF)
        return char_rom[addr];
    else
        return 0xFF;
}

void Mapper::do_write_char(uint16_t addr, uint8_t data)
{
}

static uint8_t read_000(const Mapper *m, uint16_t addr)
{
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        return m->char_rom[addr];
    }
    else if (addr >= 0x8000 && addr <= 0xFFFF) {
        const uint16_t a = addr & (prog_nbanks == 1 ? 0x3FFF : 0x7FFF);
        return m->prog_rom[a];
    }
    else {
        return 0xFF;
    }
}

static void write_000(Mapper *m, uint16_t addr, uint8_t data)
{
}

static void init_mapper_000(void)
{
}

static void finish_mapper_000(void)
{
}

static void open_mapper_000(Mapper *m, size_t prog_size, size_t char_size)
{
    m->read_func = read_000;
    m->write_func = write_000;
    m->init_func = init_mapper_000;
    m->finish_func = finish_mapper_000;

    prog_nbanks = prog_size / 0x4000; /* 16KB */
    char_nbanks = char_size / 0x2000; /* 8KB */
}

int open_mapper(Mapper *m, int id, size_t prog_size, size_t char_size)
{
    /* initialize with mapper 0 */
    open_mapper_000(m, prog_size, char_size);

    switch (id) {
    case 0:
        open_mapper_000(m, prog_size, char_size);
        break;

        /*
    case 2:
        open_mapper_002(m, prog_size, char_size);
        break;
        */

    default:
        return -1;
    }

    if (m->init_func)
        m->init_func();

    return 0;
}

void close_mapper(Mapper *m)
{
    if (m->finish_func)
        m->finish_func();
}

} // namespace
