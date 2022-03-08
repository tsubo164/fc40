CC      = cc
DEF     = -D GL_SILENCE_DEPRECATION
OPT     = -O2
CFLAGS  = $(DEF) $(OPT) -Wall --pedantic-errors -c
LDFLAGS = -lglfw -framework Cocoa -framework OpenGL -framework IOKit
RM      = rm -f

SRCS    := framebuffer main

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
	./$(NES)

$(DEPS): %.d: %.c
	$(CC) -I$(incdir) -c -MM $< > $@

ifneq "$(MAKECMDGOALS)" "clean"
-include $(DEPS)
endif
