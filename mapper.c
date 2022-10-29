#include <stdio.h>
#include "mapper.h"

static int prog_nbanks = 1;
static int char_nbanks = 1;

static int map_prog_addr_0(uint16_t addr, uint32_t *mapped)
{
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        *mapped = addr & (prog_nbanks == 1 ? 0x3FFF : 0x7FFF);
        return 1;
    }
    else {
        return 0;
    }
}

static int map_char_addr_0(uint16_t addr, uint32_t *mapped)
{
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        *mapped = addr;
        return 1;
    }
    else {
        *mapped = 0xFF;
        return 0;
    }
}

static void init_mapper_0(void)
{
    /* does nothing */
}

static void finish_mapper_0(void)
{
    /* does nothing */
}

static void open_mapper_0(struct mapper *m, size_t prog_size, size_t char_size)
{
    m->map_char_addr_func = map_char_addr_0;
    m->map_prog_addr_func = map_prog_addr_0;
    m->init_func = init_mapper_0;
    m->finish_func = finish_mapper_0;

    prog_nbanks = prog_size / 0x4000; /* 16KB */
    char_nbanks = char_size / 0x2000; /* 8KB */
}

int open_mapper(struct mapper *m, int id, size_t prog_size, size_t char_size)
{
    /* initialize with mapper 0 */
    open_mapper_0(m, prog_size, char_size);

    switch (id) {
    case 0:
        open_mapper_0(m, prog_size, char_size);
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

int map_prog_addr(const struct mapper *m, uint16_t addr, uint32_t *mapped)
{
    if (!m->map_prog_addr_func)
        return 0;
    return m->map_prog_addr_func(addr, mapped);
}

int map_char_addr(const struct mapper *m, uint16_t addr, uint32_t *mapped)
{
    if (!m->map_char_addr_func)
        return 0;
    return m->map_char_addr_func(addr, mapped);
}
