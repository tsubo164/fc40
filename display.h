#ifndef DISPLAY_H
#define DISPLAY_H

struct framebuffer;

extern int open_display(const struct framebuffer *fb, void (*update_frame_func)(void));

#endif /* _H */
