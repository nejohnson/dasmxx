DIFF ?= diff -u

test: test.out cfg.out
	$(DIFF) test.expected test.out
	$(DIFF) cfg.expected cfg.out

test.out: test.hex test.d51 ../../src/dasm51
	../../src/dasm51 -s test.d51 | sed '$${/^$$/d;}' > test.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d51 ../../src/dasm51
	../../src/dasm51 -C cfg.d51 > cfg.out

legacy:
	../../src/dasm51 legacy.d51

clean:
	rm -f test.out cfg.out cfg.bin
