#ifndef SOUND_H
#define SOUND_H

#include <cstdint>

namespace nes {

extern void InitSound();
extern void FinishSound();

extern void PushSample(float sample);
extern void SendSamples();
extern void PlaySamples();
extern void PauseSamples();

} // namespace

#endif // _H
