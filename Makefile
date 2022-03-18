CC      = cc
DEF     = -D GL_SILENCE_DEPRECATION
OPT     = -O2
CFLAGS  = $(DEF) $(OPT) -Wall --pedantic-errors -c
LDFLAGS = -lglfw -framework Cocoa -framework OpenGL -framework IOKit
RM      = rm -f

SRCS    := cartridge cpu display framebuffer main ppu

.PHONY: clean run

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

run: $(NES)
	./$(NES) sample1.nes

$(DEPS): %.d: %.c
	$(CC) -I$(incdir) -c -MM $< > $@

ifneq "$(MAKECMDGOALS)" "clean"
-include $(DEPS)
endif
