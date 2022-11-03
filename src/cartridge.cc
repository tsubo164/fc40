#include <iostream>
#include <fstream>
#include "cartridge.h"
#include "mapper.h"

namespace nes {

static void read_data(std::ifstream &ifs, std::vector<uint8_t> &v, size_t count)
{
    v.resize(count, 0);
    ifs.read(reinterpret_cast<char*>(&v[0]), sizeof(v[0]) * count);
}

bool Cartridge::Open(const char *filename)
{
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs)
        return false;

    char header[16] = {'\0'};

    ifs.read(header, sizeof(char) * 16);
    if (header[0] != 'N' ||
        header[1] != 'E' ||
        header[2] != 'S' ||
        header[3] != 0x1a)
        return false;

    prog_size_ = header[4] * 16 * 1024;
    char_size_ = header[5] * 8 * 1024;
    mirroring_ = header[6] & 0x01;
    mapper_id_ = header[6] >> 4;
    read_data(ifs, prog_rom_, prog_size_);
    read_data(ifs, char_rom_, char_size_);

    mapper_ = new_mapper(mapper_id_,
            &prog_rom_[0], prog_size_, &char_rom_[0], char_size_);

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

} // namespace
