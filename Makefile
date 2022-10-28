CC      := cc
DEF     := -D GL_SILENCE_DEPRECATION
OPT     := -O2
INCLUDE := -I/usr/local/Cellar/openal-soft/1.22.1/include/AL
LIBRARY := -L/usr/local/Cellar/openal-soft/1.22.1/lib -lopenal
CFLAGS  := $(DEF) $(OPT) $(INCLUDE) -Wall --pedantic-errors -c
LDFLAGS := -lglfw -framework Cocoa -framework OpenGL -framework IOKit $(LIBRARY)
RM      := rm -f

SRCS    := apu cartridge cpu debug display framebuffer log main mapper nes ppu sound

.PHONY: clean test

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

test: $(NES)
	$(MAKE) -C tests $@

$(DEPS): %.d: %.c
	$(CC) $(INCLUDE) -c -MM $< > $@

ifneq "$(MAKECMDGOALS)" "clean"
-include $(DEPS)
endif
