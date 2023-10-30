#ifndef DEBUG_H
#define DEBUG_H

namespace nes {

class FrameBuffer;
class Cartridge;
class CPU;
class PPU;
class NES;

void PrintCpuStatus(const CPU &cpu, const PPU &ppu);
void LogCpuStatus(NES &nes, int max_lines);

void LoadPatternTable(FrameBuffer &fb, const Cartridge *cart);
void LoadOamTable(FrameBuffer &fb, const Cartridge *cart, const PPU *ppu);

} // namespace

#endif // _H
