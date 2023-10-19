#ifndef SOUND_H
#define SOUND_H

#include <cstdint>

namespace nes {

class APU;
class NES;

extern void InitSound();
extern void FinishSound();

extern void PushSample(float sample);
extern void SendSamples();
extern void PlaySamples();
extern void PauseSamples();

void SetAPU(APU *apu);
int GetQueuedSamples();

} // namespace

#endif // _H
