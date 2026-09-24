DIFF ?= diff -u

test: test.out cfg.out issue23.out
	$(DIFF) test.expected test.out
	$(DIFF) cfg.expected cfg.out
	$(DIFF) issue23.expected issue23.out

test.out: test.hex test.d09 ../../src/dasm09
	../../src/dasm09 -s test.d09 | sed '$${/^$$/d;}' > test.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d09 ../../src/dasm09
	../../src/dasm09 -C cfg.d09 > cfg.out

issue23.bin: issue23.txt ../../src/txt2bin
	../../src/txt2bin issue23.txt issue23.bin

issue23.out: issue23.bin issue23.d09 ../../src/dasm09
	../../src/dasm09 -s issue23.d09 | sed '$${/^$$/d;}' > issue23.out

clean:
	rm -f test.out cfg.out cfg.bin issue23.out issue23.bin
