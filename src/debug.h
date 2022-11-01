#ifndef DEBUG_H
#define DEBUG_H

namespace nes {

class FrameBuffer;
struct cartridge;
struct CPU;

extern void LogCpuStatus(struct CPU *cpu, int max_lines);

extern void LoadPatternTable(FrameBuffer &fb, const struct cartridge *cart);

} // namespace

#endif // _H
