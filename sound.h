#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

extern void init_sound(void);
extern void finish_sound(void);
extern void play_sound(void);

extern void send_samples(void);

extern void push_sample(int16_t sample);

#endif /* _H */
