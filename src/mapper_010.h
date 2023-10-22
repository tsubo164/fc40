#ifndef MAPPER_010_H
#define MAPPER_010_H

#include "mapper.h"
#include "bank_map.h"

namespace nes {

class Mapper_010 : public Mapper {
public:
    Mapper_010(const std::vector<uint8_t> &prg_rom,
            const std::vector<uint8_t> &chr_rom);
    virtual ~Mapper_010();

private:
    bank_map<Size::_16KB,2> prg_;
    bank_map<Size::_4KB,2> chr_fd_;
    bank_map<Size::_4KB,2> chr_fe_;

    uint8_t latch_0_ = 0;
    uint8_t latch_1_ = 0;

    // serialization
    void do_serialize(Archive &ar) override final
    {
        SERIALIZE(ar, this, prg_);
        SERIALIZE(ar, this, chr_fd_);
        SERIALIZE(ar, this, chr_fe_);
        SERIALIZE(ar, this, latch_0_);
        SERIALIZE(ar, this, latch_1_);
    }

    uint8_t do_read_prg(uint16_t addr) const override final;
    uint8_t do_read_chr(uint16_t addr) const override final;
    void do_write_prg(uint16_t addr, uint8_t data) override final;
    void do_write_chr(uint16_t addr, uint8_t data) override final;
};

} // namespace

#endif // _H
