#include <stdio.h>
#include "mapper.h"
#include "mapper_002.h"

static int prog_nbanks = 1;
static int char_nbanks = 1;

static int32_t map_prog_addr_000(uint16_t addr)
{
    if (addr >= 0x8000 && addr <= 0xFFFF)
        return addr & (prog_nbanks == 1 ? 0x3FFF : 0x7FFF);
    else
        return -1;
}

static int32_t map_char_addr_000(uint16_t addr)
{
    if (addr >= 0x0000 && addr <= 0x1FFF)
        return addr;
    else
        return -1;
}

static uint8_t read_000(const struct mapper *m, uint16_t addr)
{
    if (addr >= 0x0000 && addr <= 0x1FFF)
        return m->char_rom[addr];

    return 0;
}

static void write_000(struct mapper *m, uint16_t addr, uint8_t data)
{
}

static void init_mapper_000(void)
{
}

static void finish_mapper_000(void)
{
}

static void open_mapper_000(struct mapper *m, size_t prog_size, size_t char_size)
{
    m->map_char_addr_func = map_char_addr_000;
    m->map_prog_addr_func = map_prog_addr_000;
    m->read_func = read_000;
    m->write_func = write_000;
    m->init_func = init_mapper_000;
    m->finish_func = finish_mapper_000;

    prog_nbanks = prog_size / 0x4000; /* 16KB */
    char_nbanks = char_size / 0x2000; /* 8KB */
}

int open_mapper(struct mapper *m, int id, size_t prog_size, size_t char_size)
{
    /* initialize with mapper 0 */
    open_mapper_000(m, prog_size, char_size);

    switch (id) {
    case 0:
        open_mapper_000(m, prog_size, char_size);
        break;

    case 2:
        open_mapper_002(m, prog_size, char_size);
        break;

    default:
        return -1;
    }

    if (m->init_func)
        m->init_func();

    return 0;
}

void close_mapper(struct mapper *m)
{
    if (m->finish_func)
        m->finish_func();
}

int32_t map_prog_addr(const struct mapper *m, uint16_t addr)
{
    if (!m->map_prog_addr_func)
        return 0;
    return m->map_prog_addr_func(addr);
}

int32_t map_char_addr(const struct mapper *m, uint16_t addr)
{
    if (!m->map_char_addr_func)
        return 0;
    return m->map_char_addr_func(addr);
}

uint8_t read_mapper(const struct mapper *m, uint16_t addr)
{
    if (!m->read_func)
        return 0;
    return m->read_func(m, addr);
}

void write_mapper(struct mapper *m, uint16_t addr, uint8_t data)
{
    if (!m->write_func)
        return;
    m->write_func(m, addr, data);
}
