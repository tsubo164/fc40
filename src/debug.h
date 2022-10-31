#ifndef DEBUG_H
#define DEBUG_H

namespace nes {

class FrameBuffer;
struct cartridge;
struct CPU;

extern void log_cpu_status(struct CPU *cpu, int max_lines);

extern void load_pattern_table(FrameBuffer &fb, const struct cartridge *cart);

} // namespace

#endif // _H
