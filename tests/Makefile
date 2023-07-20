RM      = rm -f

.PHONY: cpu_test clean test

test: cpu_test

cpu_test: ../nes
	../nes --test-mode ./nestest.nes | head -8980 > test.log
	head -8980 nestest.log | sed -e 's/ISB/ISC/' | diff - test.log
	@echo "\033[0;32mOK\033[0;39m"
	../nes ./nestest.nes

clean:
	$(RM) test.log
