DEF     = -D GL_SILENCE_DEPRECATION
OPT     = -O2
CFLAGS  = $(DEF) $(OPT) -Wall --pedantic-errors -c
LDFLAGS = -lglfw -framework Cocoa -framework OpenGL -framework IOKit

.PHONY: clean run

a.out: main.c
	cc $(DEF) $(OPT) -Wall --pedantic-errors main.c $(LDFLAGS)

run: a.out
	./a.out

clean:
	rm -f a.out
