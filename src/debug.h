#ifndef DEBUG_H
#define DEBUG_H

namespace nes {

class FrameBuffer;
class Cartridge;
struct CPU;

extern void LogCpuStatus(struct CPU *cpu, int max_lines);

extern void LoadPatternTable(FrameBuffer &fb, const Cartridge *cart);

} // namespace

#endif // _H
