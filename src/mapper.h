#ifndef MAPPER_H
#define MAPPER_H

#include <cstdint>
#include <cstdlib>
#include <string>
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
    Mapper(const std::vector<uint8_t> &prg_rom,
            const std::vector<uint8_t> &chr_rom);
    virtual ~Mapper();

    uint8_t ReadPrg(uint16_t addr) const;
    uint8_t ReadChr(uint16_t addr) const;
    void WritePrg(uint16_t addr, uint8_t data);
    void WriteChr(uint16_t addr, uint8_t data);

    uint8_t PeekPrg(uint32_t physical_addr) const;

    size_t GetPrgRomSize() const;
    size_t GetChrRomSize() const;
    size_t GetPrgRamSize() const;
    size_t GetChrRamSize() const;

    std::vector<uint8_t> GetPrgRam() const;
    void SetPrgRam(const std::vector<uint8_t> &sram);

    Mirroring GetMirroring() const;
    void SetMirroring(Mirroring mirroring);

    std::string GetBoardName() const;

protected:
    uint8_t read_prg_rom(uint32_t index) const;
    uint8_t read_chr_rom(uint32_t index) const;
    uint8_t read_prg_ram(uint32_t index) const;
    uint8_t read_chr_ram(uint32_t index) const;

    void write_prg_ram(uint32_t index, uint8_t data);
    void write_chr_ram(uint32_t index, uint8_t data);

    void use_prg_ram(uint32_t size);
    void use_chr_ram(uint32_t size);

    void set_board_name(const std::string &name);

private:
    std::string board_name_ = "";
    std::vector<uint8_t> prg_rom_;
    std::vector<uint8_t> chr_rom_;
    std::vector<uint8_t> prg_ram_;
    std::vector<uint8_t> chr_ram_;

    Mirroring mirroring_ = Mirroring::HORIZONTAL;

    virtual uint8_t do_read_prg(uint16_t addr) const = 0;
    virtual uint8_t do_read_chr(uint16_t addr) const = 0;
    virtual void do_write_prg(uint16_t addr, uint8_t data) = 0;
    virtual void do_write_chr(uint16_t addr, uint8_t data) = 0;
};

extern std::shared_ptr<Mapper> new_mapper(int id,
        const std::vector<uint8_t> &prg_data,
        const std::vector<uint8_t> &chr_data);

} // namespace

#endif // _H
