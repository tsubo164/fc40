.PHONY: clean run

a.out: main.c
	cc -O2 -Wall --pedantic-errors main.c

run: a.out
	./a.out

clean:
	rm -f a.out
