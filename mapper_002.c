#include <stdio.h>
#include "mapper_002.h"

static int prog_nbanks = 1;
static int char_nbanks = 1;

static int prog_bank = 0;
static int prog_fixed = 0;

static uint8_t char_ram[0x2000] = {0x00};

static int32_t map_prog_addr_002(uint16_t addr)
{
    if (addr >= 0x8000 && addr <= 0xBFFF)
        return prog_bank * 0x4000 + (addr & 0x3FFF);
    else if (addr >= 0xC000 && addr <= 0xFFFF)
        return prog_fixed * 0x4000 + (addr & 0x3FFF);

    return -1;
}

static int32_t map_char_addr_002(uint16_t addr)
{
    if (addr >= 0x0000 && addr <= 0x1FFF)
        return addr;
    else
        return -1;
}

static uint8_t read_002(const struct mapper *m, uint16_t addr)
{
	if (addr >= 0x0000 && addr <= 0x1FFF)
		return char_ram[addr];

    return 0xFF;
}

static void write_002(struct mapper *m, uint16_t addr, uint8_t data)
{
	if (addr >= 0x0000 && addr <= 0x1FFF)
		char_ram[addr] = data;
    else if (addr >= 0x8000 && addr <= 0xFFFF)
		prog_bank = data & 0x0F;
}

static void init_mapper_002(void)
{
}

static void finish_mapper_002(void)
{
}

void open_mapper_002(struct mapper *m, size_t prog_size, size_t char_size)
{
    m->map_char_addr_func = map_char_addr_002;
    m->map_prog_addr_func = map_prog_addr_002;
    m->read_func = read_002;
    m->write_func = write_002;
    m->init_func = init_mapper_002;
    m->finish_func = finish_mapper_002;

    prog_nbanks = prog_size / 0x4000; /* 16KB */
    char_nbanks = char_size / 0x2000; /* 8KB */

    prog_bank = 0;
	prog_fixed = prog_nbanks - 1;
}
