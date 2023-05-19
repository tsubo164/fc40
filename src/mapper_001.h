#ifndef MAPPER_001_H
#define MAPPER_001_H

#include "mapper.h"

namespace nes {

class Mapper_001 : public Mapper {
public:
    Mapper_001(const std::vector<uint8_t> &prog_rom,
            const std::vector<uint8_t> &char_rom);
    virtual ~Mapper_001();

private:
    int prog_nbanks_ = 1;
    int char_nbanks_ = 1;

    uint8_t shift_register_ = 0x00;

    uint8_t mirror_ = 0;
    uint8_t prog_bank_mode_ = 0;
    uint8_t char_bank_mode_ = 0;

    int prog_bank_0_ = 0;
    int prog_bank_1_ = 0;
    int char_bank_0_ = 0;
    int char_bank_1_ = 0;
    uint8_t prog_ram_[0x2000] = {0x00};

    void set_control();
    void set_prog_bank();
    void set_char_bank_0();
    void set_char_bank_1();

    virtual uint8_t do_read_prog(uint16_t addr) const final;
    virtual void do_write_prog(uint16_t addr, uint8_t data) final;
    virtual uint8_t do_read_char(uint16_t addr) const final;
    virtual void do_write_char(uint16_t addr, uint8_t data) final;
};

} // namespace

#endif // _H
