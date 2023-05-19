#include <iostream>
#include <fstream>
#include "cartridge.h"
#include "mapper.h"

namespace nes {

static std::vector<uint8_t> read_data(std::ifstream &ifs, size_t count)
{
    std::vector<uint8_t> data(count, 0);
    ifs.read(reinterpret_cast<char*>(&data[0]), sizeof(data[0]) * count);
    return data;
}

bool Cartridge::Open(const char *filename)
{
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs)
        return false;

    char header[16] = {'\0'};

    // 0-3 Constant $4E $45 $53 $1A (ASCII "NES" followed by MS-DOS end-of-file)
    ifs.read(header, sizeof(char) * 16);
    if (header[0] != 'N' ||
        header[1] != 'E' ||
        header[2] != 'S' ||
        header[3] != 0x1a)
        return false;

    // 4 Size of PRG ROM in 16 KB units
    // 5 Size of CHR ROM in 8 KB units (value 0 means the board uses CHR RAM)
    const size_t prog_size = header[4] * 16 * 1024;
    const size_t char_size = header[5] * 8 * 1024;

    // 6
    // 76543210
    // ||||||||
    // |||||||+- Mirroring: 0: horizontal (vertical arrangement) (CIRAM A10 = PPU A11)
    // |||||||              1: vertical (horizontal arrangement) (CIRAM A10 = PPU A10)
    // ||||||+-- 1: Cartridge contains battery-backed PRG RAM ($6000-7FFF) or other persistent memory
    // |||||+--- 1: 512-byte trainer at $7000-$71FF (stored before PRG data)
    // ||||+---- 1: Ignore mirroring control or above mirroring bit; instead provide four-screen VRAM
    // ++++----- Lower nybble of mapper number
    mirroring_   = (header[6] >> 0) & 0x01;
    has_battery_ = (header[6] >> 1) & 0x01;
    mapper_id_   = (header[6] >> 4);

    const std::vector<uint8_t> prog_data = read_data(ifs, prog_size);
    const std::vector<uint8_t> char_data = read_data(ifs, char_size);

    if (mapper_id_ == 0 || mapper_id_ == 2 || mapper_id_ == 3) {
    mapper_ = new_mapper(mapper_id_,
            &prog_data[0], prog_size, &char_data[0], char_size);
    }
    else {
    mapper_ = new_mapper(mapper_id_,
            &prog_data[0], prog_size, &char_data[0], char_size);

    // Set up ROMs and RAMs
    mapper_->LoadProgData(prog_data);
    mapper_->LoadCharData(char_data);
    }

    return true;
}

uint8_t Cartridge::ReadProg(uint16_t addr) const
{
    return mapper_->ReadProg(addr);
}

void Cartridge::WriteProg(uint16_t addr, uint8_t data)
{
    mapper_->WriteProg(addr, data);
}

uint8_t Cartridge::ReadChar(uint16_t addr) const
{
    return mapper_->ReadChar(addr);
}

void Cartridge::WriteChar(uint16_t addr, uint8_t data)
{
    mapper_->WriteChar(addr, data);
}

uint8_t Cartridge::PeekProg(uint32_t physical_addr) const
{
    return mapper_->PeekProg(physical_addr);
}

int Cartridge::GetMapperID() const
{
    return mapper_id_;
}

size_t Cartridge::GetProgSize() const
{
    return mapper_->GetProgRomSize();
}

size_t Cartridge::GetCharSize() const
{
    return mapper_->GetCharRomSize();
}

bool Cartridge::IsMapperSupported() const
{
    return mapper_ != nullptr;
}

bool Cartridge::IsVerticalMirroring() const
{
    return mirroring_ == 1;
}

bool Cartridge::HasBattery() const
{
    return has_battery_;
}

} // namespace
