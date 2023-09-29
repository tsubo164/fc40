CC      := g++
DEF     := -D GL_SILENCE_DEPRECATION
OPT     := -O2
INCLUDE := -I/usr/local/Cellar/openal-soft/1.22.2/include/AL
LIBRARY := -L/usr/local/Cellar/openal-soft/1.22.2/lib -lopenal
CFLAGS  := $(DEF) $(OPT) $(INCLUDE) -Wall --pedantic-errors --std=c++14 -c
LDFLAGS := -lglfw -framework Cocoa -framework OpenGL -framework IOKit $(LIBRARY)
RM      := rm -f

SRCS    := apu cartridge cpu debug disassemble display dma framebuffer instruction \
           main mapper mapper_000 mapper_001 mapper_002 mapper_003 mapper_004 \
           mapper_019 nes ppu sound

.PHONY: clean test

NES  := ../nes
OBJS := $(addsuffix .o, $(SRCS))
DEPS := $(addsuffix .d, $(SRCS))

all: $(NES)

$(OBJS): %.o: %.cc
	$(CC) $(CFLAGS) -o $@ $<

$(NES): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(NES) *.o *.d

test: $(NES)
	$(MAKE) -C tests $@

$(DEPS): %.d: %.cc
	$(CC) $(INCLUDE) -c -MM $< > $@

ifneq "$(MAKECMDGOALS)" "clean"
-include $(DEPS)
endif
