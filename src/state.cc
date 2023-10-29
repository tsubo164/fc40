#include "state.h"
#include "serialize.h"
#include "nes.h"
#include <fstream>

namespace nes {

bool SaveState(NES &nes, const std::string &filename)
{
    std::ofstream ofs(filename);
    if (!ofs)
        return false;

    Archive ar;

    Serialize(ar, "nes", &nes);
    ar.Write(ofs);

    std::cout << filename << ": saved successfully" << std::endl;
    return true;
}

bool LoadState(NES &nes, const std::string &filename)
{
    std::ifstream ifs(filename);
    if (!ifs)
        return false;

    Archive ar;

    Serialize(ar, "nes", &nes);
    ar.Read(ifs);

    std::cout << filename << ": loaded successfully" << std::endl;
    return true;
}

} // namespace
