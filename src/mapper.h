#ifndef MAPPER_H
#define MAPPER_H

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <memory>

namespace nes {

enum class Mirroring {
    HORIZONTAL,
    VERTICAL,
    SINGLE_SCREEN,
    FOUR_SCREEN,
};

class Mapper {
public:
    Mapper(const std::vector<uint8_t> &prog_rom,
            const std::vector<uint8_t> &char_rom);
    virtual ~Mapper();

    uint8_t ReadProg(uint16_t addr) const;
    void WriteProg(uint16_t addr, uint8_t data);
    uint8_t ReadChar(uint16_t addr) const;
    void WriteChar(uint16_t addr, uint8_t data);

    uint8_t PeekProg(uint32_t physical_addr) const;

    size_t GetProgRomSize() const;
    size_t GetCharRomSize() const;
    size_t GetProgRamSize() const;
    size_t GetCharRamSize() const;

    std::vector<uint8_t> GetProgRam() const;
    void SetProgRam(const std::vector<uint8_t> &sram);

    Mirroring GetMirroring() const;
    void SetMirroring(Mirroring mirroring);

protected:
    uint8_t read_prog_rom(uint32_t addr) const;
    uint8_t read_char_rom(uint32_t addr) const;
    uint8_t read_prog_ram(uint32_t addr) const;
    uint8_t read_char_ram(uint32_t addr) const;

    void write_prog_ram(uint32_t addr, uint8_t data);
    void write_char_ram(uint32_t addr, uint8_t data);

    void use_prog_ram(uint32_t size);
    void use_char_ram(uint32_t size);

private:
    std::vector<uint8_t> prog_rom_;
    std::vector<uint8_t> char_rom_;
    std::vector<uint8_t> prog_ram_;
    std::vector<uint8_t> char_ram_;

    Mirroring mirroring_ = Mirroring::HORIZONTAL;

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
