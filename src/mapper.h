#ifndef MAPPER_H
#define MAPPER_H

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <memory>

namespace nes {

class Mapper {
public:
    Mapper(const std::vector<uint8_t> &prog_rom,
            const std::vector<uint8_t> &char_rom);
    virtual ~Mapper();

    void LoadProgData(const std::vector<uint8_t> &data);
    void LoadCharData(const std::vector<uint8_t> &data);

    uint8_t ReadProg(uint16_t addr) const;
    void WriteProg(uint16_t addr, uint8_t data);
    uint8_t ReadChar(uint16_t addr) const;
    void WriteChar(uint16_t addr, uint8_t data);

    uint8_t PeekProg(uint32_t physical_addr) const;

    size_t GetProgRomSize() const;
    size_t GetCharRomSize() const;

protected:
    uint8_t read_prog_rom(uint32_t addr) const;
    uint8_t read_char_rom(uint32_t addr) const;

private:
    std::vector<uint8_t> prog_rom_;
    std::vector<uint8_t> char_rom_;

    virtual uint8_t do_read_prog(uint16_t addr) const = 0;
    virtual void do_write_prog(uint16_t addr, uint8_t data) = 0;
    virtual uint8_t do_read_char(uint16_t addr) const = 0;
    virtual void do_write_char(uint16_t addr, uint8_t data) = 0;
};

extern std::shared_ptr<Mapper> new_mapper(int id,
        const std::vector<uint8_t> &prog_data,
        const std::vector<uint8_t> &char_data);

} // namespace

#endif // _H
