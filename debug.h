#ifndef DEBUG_H
#define DEBUG_H

struct framebuffer;
struct cartridge;

extern void load_pattern_table(struct framebuffer *fb, const struct cartridge *cart);

#endif /* _H */
