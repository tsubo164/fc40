#include <stdio.h>
#include "mapper_002.h"

namespace nes {

static int prog_nbanks = 1;
static int char_nbanks = 1;

static int prog_bank = 0;
static int prog_fixed = 0;

static uint8_t char_ram[0x2000] = {0x00};

static uint8_t read_002(const struct mapper *m, uint16_t addr)
{
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        return char_ram[addr];
    }
    else if (addr >= 0x8000 && addr <= 0xBFFF) {
        const uint32_t a = prog_bank * 0x4000 + (addr & 0x3FFF);
        return m->prog_rom[a];
    }
    else if (addr >= 0xC000 && addr <= 0xFFFF) {
        const uint32_t a = prog_fixed * 0x4000 + (addr & 0x3FFF);
        return m->prog_rom[a];
    }
    else {
        return 0xFF;
    }

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
    m->read_func = read_002;
    m->write_func = write_002;
    m->init_func = init_mapper_002;
    m->finish_func = finish_mapper_002;

    prog_nbanks = prog_size / 0x4000; /* 16KB */
    char_nbanks = char_size / 0x2000; /* 8KB */

    prog_bank = 0;
    prog_fixed = prog_nbanks - 1;
}

} // namespace
