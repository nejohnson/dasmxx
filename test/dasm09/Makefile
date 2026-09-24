DIFF ?= diff -u

test: test.out cfg.out
	$(DIFF) test.expected test.out
	$(DIFF) cfg.expected cfg.out

test.out: test.hex test.d09 ../../src/dasm09
	../../src/dasm09 -s test.d09 | sed '$${/^$$/d;}' > test.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d09 ../../src/dasm09
	../../src/dasm09 -C cfg.d09 > cfg.out

clean:
	rm -f test.out cfg.out cfg.bin
