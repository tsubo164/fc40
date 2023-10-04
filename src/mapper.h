#ifndef MAPPER_H
#define MAPPER_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <array>
#include "bank_map.h"

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
    uint8_t ReadNameTable(uint16_t addr) const;
    void WritePrg(uint16_t addr, uint8_t data);
    void WriteChr(uint16_t addr, uint8_t data);
    void WriteNameTable(uint16_t addr, uint8_t data);

    uint8_t PeekPrg(uint32_t physical_addr) const;

    size_t GetPrgRomSize() const;
    size_t GetChrRomSize() const;
    size_t GetPrgRamSize() const;
    size_t GetChrRamSize() const;

    std::vector<uint8_t> GetPrgRam() const;
    void SetPrgRam(const std::vector<uint8_t> &sram);
    void SetNameTable(std::array<uint8_t,2048> *nt);

    bool IsSetIRQ() const;
    void ClearIRQ();
    void PpuClock(int cycle, int scanline);
    void CpuClock();

    Mirroring GetMirroring() const;
    void SetMirroring(Mirroring mirroring);

    std::string GetBoardName() const;

protected:
    // TODO change uint32_t to uint16_t
    uint8_t read_prg_rom(uint32_t index) const;
    uint8_t read_chr_rom(uint32_t index) const;
    uint8_t read_prg_ram(uint32_t index) const;
    uint8_t read_chr_ram(uint32_t index) const;
    uint8_t read_nametable(uint16_t addr) const;

    // TODO change uint32_t to uint16_t
    void write_prg_ram(uint32_t index, uint8_t data);
    void write_chr_ram(uint32_t index, uint8_t data);
    void write_nametable(uint16_t addr, uint8_t data);

    // TODO change uint32_t to uint16_t
    void use_prg_ram(uint32_t size);
    void use_chr_ram(uint32_t size);

    // TODO remove this after switching to new APIs
    bool new_api = false;
    void set_prg_bank_size(Size bank_size);
    void set_chr_bank_size(Size bank_size);
    void set_chr_bank_size(Size bank_size, uint16_t window_count);
    void select_prg_bank(uint16_t window_base, int16_t bank_index);
    void select_chr_bank(uint16_t window_base, int16_t bank_index);

    void set_board_name(const std::string &name);
    void set_irq();

private:
    std::string board_name_ = "";
    std::vector<uint8_t> prg_rom_;
    std::vector<uint8_t> chr_rom_;
    std::vector<uint8_t> prg_ram_;
    std::vector<uint8_t> chr_ram_;
    std::array<uint8_t,2048> *nametable_ = nullptr;

    BankMap prg_banks_;
    BankMap chr_banks_;

    Mirroring mirroring_ = Mirroring::HORIZONTAL;
    bool irq_generated_ = false;

    virtual uint8_t do_read_prg(uint16_t addr) const = 0;
    virtual uint8_t do_read_chr(uint16_t addr) const = 0;
    virtual uint8_t do_read_nametable(uint16_t addr) const;
    virtual void do_write_prg(uint16_t addr, uint8_t data) = 0;
    virtual void do_write_chr(uint16_t addr, uint8_t data) = 0;
    virtual void do_write_nametable(uint16_t addr, uint8_t data);

    virtual void do_ppu_clock(int cycle, int scanline) {}
    virtual void do_cpu_clock() {}

    uint16_t nametable_index(uint16_t addr) const;
};

extern std::shared_ptr<Mapper> new_mapper(int id,
        const std::vector<uint8_t> &prg_data,
        const std::vector<uint8_t> &chr_data);

} // namespace

#endif // _H
