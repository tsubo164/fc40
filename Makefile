.PHONY: clean

a.out: main.c
	cc -O2 -Wall -ansi --pedantic-errors main.c

clean:
	rm -f a.out
