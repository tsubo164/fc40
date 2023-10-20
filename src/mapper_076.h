#ifndef MAPPER_076_H
#define MAPPER_076_H

#include "mapper.h"
#include "bank_map.h"

namespace nes {

class Mapper_076 : public Mapper {
public:
    Mapper_076(const std::vector<uint8_t> &prg_rom,
            const std::vector<uint8_t> &chr_rom);
    virtual ~Mapper_076();

private:
    bank_map<Size::_8KB,4> prg_;
    bank_map<Size::_2KB,4> chr_;

    uint8_t bank_select_ = 0;

    // serialization
    void do_serialize(Archive &ar) override final
    {
        SERIALIZE(ar, this, prg_);
        SERIALIZE(ar, this, chr_);
        SERIALIZE(ar, this, bank_select_);
    }

    uint8_t do_read_prg(uint16_t addr) const override final;
    uint8_t do_read_chr(uint16_t addr) const override final;
    void do_write_prg(uint16_t addr, uint8_t data) override final;
    void do_write_chr(uint16_t addr, uint8_t data) override final;
};

} // namespace

#endif // _H
