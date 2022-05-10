#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

struct framebuffer;

struct display {
    struct framebuffer *fb;
    struct framebuffer *pattern_table;

    void (*update_frame_func)(void);
    void (*input_controller_func)(uint8_t id, uint8_t input);
};

extern int open_display(const struct display *disp);

#endif /* _H */
