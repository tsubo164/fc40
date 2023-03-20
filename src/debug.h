#ifndef DEBUG_H
#define DEBUG_H

namespace nes {

class FrameBuffer;
class Cartridge;
class CPU;
class PPU;
class NES;

extern void PrintCpuStatus(const CPU &cpu, const PPU &ppu);
extern void LogCpuStatus(NES &nes, int max_lines);

extern void LoadPatternTable(FrameBuffer &fb, const Cartridge *cart);

} // namespace

#endif // _H
