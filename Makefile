.PHONY: all clean test

NES  := nes

all: $(NES)

$(NES):
	$(MAKE) -C src all

clean:
	$(MAKE) -C src $@
	$(MAKE) -C tests $@

test: $(NES)
	$(MAKE) -C tests $@
