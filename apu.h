#ifndef APU_H
#define APU_H

#include <stdint.h>

struct APU {
    uint8_t data;
};

/* write registers */
extern void write_apu_square1_volume(struct APU *apu, uint8_t data);
extern void write_apu_square1_sweep(struct APU *apu, uint8_t data);
extern void write_apu_square1_lo(struct APU *apu, uint8_t data);
extern void write_apu_square1_hi(struct APU *apu, uint8_t data);

/* read registers */

#endif /* _H */
