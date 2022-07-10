CC      = cc
DEF     = -D GL_SILENCE_DEPRECATION
OPT     = -O2
CFLAGS  = $(DEF) $(OPT) -Wall --pedantic-errors -c
LDFLAGS = -lglfw -framework Cocoa -framework OpenGL -framework IOKit
RM      = rm -f

SRCS    := apu cartridge cpu debug display framebuffer log main nes ppu

.PHONY: clean run test

NES  := nes
OBJS := $(addsuffix .o, $(SRCS))
DEPS := $(addsuffix .d, $(SRCS))

all: $(NES)

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

$(NES): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(NES) *.o *.d
	$(MAKE) -C tests $@

run: $(NES)
	./$(NES) sample1.nes

test: $(NES)
	$(MAKE) -C tests $@

$(DEPS): %.d: %.c
	$(CC) -I$(incdir) -c -MM $< > $@

ifneq "$(MAKECMDGOALS)" "clean"
-include $(DEPS)
endif
