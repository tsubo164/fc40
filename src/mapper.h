#ifndef MAPPER_H
#define MAPPER_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <array>
#include "serialize.h"

namespace nes {

enum class Mirroring {
    HORIZONTAL,
    VERTICAL,
    SINGLE_SCREEN_0,
    SINGLE_SCREEN_1,
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
    bool HasPrgRamWritten() const;

    bool IsSetIRQ() const;
    void ClearIRQ();
    void PpuClock(int cycle, int scanline);
    void CpuClock();

    Mirroring GetMirroring() const;
    void SetMirroring(Mirroring mirroring);

    std::string GetBoardName() const;

protected:
    uint8_t read_prg_rom(int index) const;
    uint8_t read_chr_rom(int index) const;
    uint8_t read_prg_ram(int index) const;
    uint8_t read_chr_ram(int index) const;
    uint8_t read_nametable(int index) const;

    void write_prg_ram(int index, uint8_t data);
    void write_chr_ram(int index, uint8_t data);
    void write_nametable(int index, uint8_t data);

    void use_prg_ram(int size);
    void use_chr_ram(int size);
    bool is_prg_ram_used() const;
    bool is_chr_ram_used() const;

    void set_prg_ram_protect(bool enable);
    bool is_prg_ram_protected() const;

    void set_board_name(const std::string &name);
    void set_irq();

private:
    std::string board_name_ = "";
    std::vector<uint8_t> prg_rom_;
    std::vector<uint8_t> chr_rom_;
    std::vector<uint8_t> prg_ram_;
    std::vector<uint8_t> chr_ram_;
    std::array<uint8_t,2048> *nametable_ = nullptr;

    Mirroring mirroring_ = Mirroring::HORIZONTAL;
    bool irq_generated_ = false;
    bool prg_ram_protected_ = false;
    bool prg_ram_written_ = false;

    // serialization
    friend void Serialize(Archive &ar, const std::string &name, Mapper *data)
    {
        // base mapper
        SERIALIZE_NAMESPACE_BEGIN(ar, "mapper_");
        if (!data->prg_ram_.empty())
            SERIALIZE(ar, data, prg_ram_);
        if (!data->chr_ram_.empty())
            SERIALIZE(ar, data, chr_ram_);
        SERIALIZE(ar, data, irq_generated_);
        SERIALIZE(ar, data, prg_ram_protected_);
        SERIALIZE(ar, data, prg_ram_written_);
        {
            // derived mapper
            SERIALIZE_NAMESPACE_BEGIN(ar, name);
            data->do_serialize(ar);
            SERIALIZE_NAMESPACE_END(ar);
        }
        SERIALIZE_NAMESPACE_END(ar);
    }

    virtual uint8_t do_read_prg(uint16_t addr) const = 0;
    virtual uint8_t do_read_chr(uint16_t addr) const = 0;
    virtual uint8_t do_read_nametable(uint16_t addr) const;
    virtual void do_write_prg(uint16_t addr, uint8_t data) = 0;
    virtual void do_write_chr(uint16_t addr, uint8_t data) = 0;
    virtual void do_write_nametable(uint16_t addr, uint8_t data);

    virtual void do_ppu_clock(int cycle, int scanline) {}
    virtual void do_cpu_clock() {}
    virtual void do_serialize(Archive &ar) {}

    int nametable_index(uint16_t addr) const;
};

std::shared_ptr<Mapper> new_mapper(int id,
        const std::vector<uint8_t> &prg_data,
        const std::vector<uint8_t> &chr_data);

} // namespace

#endif // _H
