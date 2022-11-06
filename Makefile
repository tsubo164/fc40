.PHONY: all clean test

all:
	$(MAKE) -C src $@

clean:
	$(MAKE) -C src $@
	$(MAKE) -C tests $@

test: all
	$(MAKE) -C tests $@
