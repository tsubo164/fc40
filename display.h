#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

struct framebuffer;

extern int open_display(const struct framebuffer *fb,
        void (*update_frame_func)(void),
        void (*input_controller_func)(uint8_t id, uint8_t input));

#endif /* _H */
