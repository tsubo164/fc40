.PHONY: all clean test

NES  := nes

all:
	$(MAKE) -C src $@

clean:
	$(MAKE) -C src $@
	$(MAKE) -C tests $@

test: $(NES)
	$(MAKE) -C tests $@
