#ifndef MAPPER_H
#define MAPPER_H

#include <cstdint>
#include <cstdlib>
#include <memory>

namespace nes {

class Mapper {
public:
    Mapper(const uint8_t *prog_rom, size_t prog_size,
           const uint8_t *char_rom, size_t char_size);
    virtual ~Mapper();

    uint8_t ReadProg(uint16_t addr) const;
    void WriteProg(uint16_t addr, uint8_t data);
    uint8_t ReadChar(uint16_t addr) const;
    void WriteChar(uint16_t addr, uint8_t data);

protected:
    uint8_t read_prog_rom(uint16_t addr) const;
    uint8_t read_char_rom(uint16_t addr) const;
    size_t prog_size() const;
    size_t char_size() const;

private:
    const uint8_t *prog_rom_ = nullptr;
    const uint8_t *char_rom_ = nullptr;
    size_t prog_size_ = 0;
    size_t char_size_ = 0;

private:
    int prog_nbanks_ = 1;
    int char_nbanks_ = 1;

    virtual uint8_t do_read_prog(uint16_t addr) const;
    virtual void do_write_prog(uint16_t addr, uint8_t data);
    virtual uint8_t do_read_char(uint16_t addr) const;
    virtual void do_write_char(uint16_t addr, uint8_t data);
};

extern std::shared_ptr<Mapper> new_mapper(int id,
        const uint8_t *prog_rom, size_t prog_size,
        const uint8_t *char_rom, size_t char_size);

} // namespace

#endif // _H
