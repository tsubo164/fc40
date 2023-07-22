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

Cartridge::Cartridge()
{
}

Cartridge::~Cartridge()
{
    const int err = save_battery_ram();
    if (err) {
        // Report an error.
    }
}

bool Cartridge::Open(const char *filename)
{
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs)
        return false;

    ines_filename_ = filename;
    char header[16] = {'\0'};

    // Bytes 0-3 Constant $4E $45 $53 $1A (ASCII "NES" followed by MS-DOS end-of-file)
    ifs.read(header, sizeof(char) * 16);
    if (header[0] != 'N' ||
        header[1] != 'E' ||
        header[2] != 'S' ||
        header[3] != 0x1a)
        return false;

    // Byte 4 Size of PRG ROM in 16 KB units
    // Byte 5 Size of CHR ROM in 8 KB units (value 0 means the board uses CHR RAM)
    const size_t prg_size = header[4] * 16 * 1024;
    const size_t chr_size = header[5] * 8 * 1024;

    // Flags 6
    // 76543210
    // ||||||||
    // |||||||+- Mirroring: 0: horizontal (vertical arrangement) (CIRAM A10 = PPU A11)
    // |||||||              1: vertical (horizontal arrangement) (CIRAM A10 = PPU A10)
    // ||||||+-- 1: Cartridge contains battery-backed PRG RAM ($6000-7FFF) or
    // ||||||       other persistent memory
    // |||||+--- 1: 512-byte trainer at $7000-$71FF (stored before PRG data)
    // ||||+---- 1: Ignore mirroring control or above mirroring bit;
    // ||||         instead provide four-screen VRAM
    // ++++----- Lower nybble of mapper number
    mirroring_   = (header[6] >> 0) & 0x01;
    has_battery_ = (header[6] >> 1) & 0x01;
    mapper_id_   = (header[6] >> 4) & 0x0F;

    // Flags 7
    // 76543210
    // ||||||||
    // |||||||+- VS Unisystem
    // ||||||+-- PlayChoice-10 (8 KB of Hint Screen data stored after CHR data)
    // ||||++--- If equal to 2, flags 8-15 are in NES 2.0 format
    // ++++----- Upper nybble of mapper number
    mapper_id_ |= header[7] & 0xF0;

    const std::vector<uint8_t> prg_data = read_data(ifs, prg_size);
    const std::vector<uint8_t> chr_data = read_data(ifs, chr_size);

    mapper_ = new_mapper(mapper_id_, prg_data, chr_data);
    if (mirroring_ == 0)
        mapper_->SetMirroring(Mirroring::HORIZONTAL);
    else if (mirroring_ == 1)
        mapper_->SetMirroring(Mirroring::VERTICAL);

    if (HasBattery()) {
        sram_filename_ = ines_filename_ + ".sram";
        load_battery_ram();
    }

    return true;
}

uint8_t Cartridge::ReadPrg(uint16_t addr) const
{
    return mapper_->ReadPrg(addr);
}

void Cartridge::WritePrg(uint16_t addr, uint8_t data)
{
    mapper_->WritePrg(addr, data);
}

uint8_t Cartridge::ReadChr(uint16_t addr) const
{
    return mapper_->ReadChr(addr);
}

void Cartridge::WriteChr(uint16_t addr, uint8_t data)
{
    mapper_->WriteChr(addr, data);
}

uint8_t Cartridge::PeekPrg(uint32_t physical_addr) const
{
    return mapper_->PeekPrg(physical_addr);
}

int Cartridge::GetMapperID() const
{
    return mapper_id_;
}

size_t Cartridge::GetPrgSize() const
{
    return mapper_->GetPrgRomSize();
}

size_t Cartridge::GetChrSize() const
{
    return mapper_->GetChrRomSize();
}

bool Cartridge::IsMapperSupported() const
{
    return mapper_ != nullptr;
}

bool Cartridge::IsVerticalMirroring() const
{
    return mapper_->GetMirroring() == Mirroring::VERTICAL;
}

bool Cartridge::HasBattery() const
{
    return has_battery_;
}

std::string Cartridge::GetBoardName() const
{
    return mapper_->GetBoardName();
}

int Cartridge::save_battery_ram() const
{
	std::ofstream ofs(sram_filename_, std::ios::binary);
	if (!ofs)
		return -1;

    const std::vector<uint8_t> sram = mapper_->GetPrgRam();
    if (sram.size() != 0x2000)
        return -1;

	ofs.write(reinterpret_cast<const char*>(&sram[0]), sram.size());
	return 0;
}

int Cartridge::load_battery_ram()
{
    std::ifstream ifs(sram_filename_, std::ios::binary);
    if (!ifs)
        return -1;

    ifs.seekg(0, ifs.end);
    const std::size_t sram_size = ifs.tellg();
    ifs.seekg(0, ifs.beg);

    if (sram_size != 0x2000)
        return -1;

    std::vector<uint8_t> sram(sram_size);
    ifs.read(reinterpret_cast<char*>(&sram[0]), sram.size());

    mapper_->SetPrgRam(sram);
    return 0;
}

} // namespace
