#ifndef DEBUG_H
#define DEBUG_H

namespace nes {

class FrameBuffer;
class Cartridge;
class CPU;

extern void LogCpuStatus(CPU *cpu, int max_lines);

extern void LoadPatternTable(FrameBuffer &fb, const Cartridge *cart);

} // namespace

#endif // _H
