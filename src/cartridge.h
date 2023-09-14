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

    uint8_t ReadPrg(uint16_t addr) const;
    uint8_t ReadChr(uint16_t addr) const;
    void WritePrg(uint16_t addr, uint8_t data);
    void WriteChr(uint16_t addr, uint8_t data);

    uint8_t PeekPrg(uint32_t physical_addr) const;

    int GetMapperID() const;
    size_t GetPrgSize() const;
    size_t GetChrSize() const;

    bool IsSetIRQ() const;
    void ClearIRQ();
    void Clock(int cycle, int scanline);

    bool IsMapperSupported() const;
    bool IsVerticalMirroring() const;
    bool HasBattery() const;
    std::string GetBoardName() const;

private:
    int mapper_id_ = -1;
    uint8_t mirroring_ = 0;
    bool has_battery_ = false;
    std::shared_ptr<Mapper> mapper_ = nullptr;

    std::string ines_filename_ = "";
    std::string sram_filename_ = "";

    int save_battery_ram() const;
    int load_battery_ram();
};

} // namespace

#endif // _H
