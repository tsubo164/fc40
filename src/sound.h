#ifndef SOUND_H
#define SOUND_H

namespace nes {

extern void InitSound();
extern void FinishSound();

extern void PushSample(float sample);
extern void SendSamples();
extern void PlaySamples();
extern void PauseSamples();

int GetQueuedSampleCount();

} // namespace

#endif // _H
