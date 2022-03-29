RM      = rm -f

.PHONY: cpu_test clean test

test: cpu_test
	@echo "\033[0;32mOK\033[0;39m"

cpu_test: ../nes
	../nes --log-mode ./nestest.nes > test.log
	head -48 nestest.log | diff - test.log

clean:
	$(RM) test.log
