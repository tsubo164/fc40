#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <cstdlib>
#include <cstdint>
#include <string>
#include <vector>
#include "mapper.h"

namespace nes {

class Cartridge {
public:
    Cartridge();
    ~Cartridge();

    bool Open(const char *filename);

    uint8_t ReadProg(uint16_t addr) const;
    void WriteProg(uint16_t addr, uint8_t data);
    uint8_t ReadChar(uint16_t addr) const;
    void WriteChar(uint16_t addr, uint8_t data);

    uint8_t PeekProg(uint32_t physical_addr) const;

    int GetMapperID() const;
    size_t GetProgSize() const;
    size_t GetCharSize() const;

    bool IsMapperSupported() const;
    bool IsVerticalMirroring() const;
    bool HasBattery() const;

private:
    uint8_t mirroring_ = 0;
    int mapper_id_ = -1;
    bool has_battery_ = false;
    std::shared_ptr<Mapper> mapper_ = nullptr;

    std::string ines_filename_ = "";
    std::string sram_filename_ = "";

    int save_battery_ram() const;
    int load_battery_ram();
};

} // namespace

#endif // _H
