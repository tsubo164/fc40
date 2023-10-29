#ifndef STATE_H
#define STATE_H

#include <string>

namespace nes {

class NES;

bool SaveState(NES &nes, const std::string &filename);
bool LoadState(NES &nes, const std::string &filename);

} // namespace

#endif // _H
