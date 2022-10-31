#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

namespace nes {

extern void init_sound(void);
extern void finish_sound(void);

extern void push_sample(float sample);
extern void send_samples(void);
extern void play_samples(void);

} // namespace

#endif // _H
