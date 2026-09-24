DIFF ?= diff -u

test: test.out cfg.out
	$(DIFF) test.expected test.out
	$(DIFF) cfg.expected cfg.out

test.bin: test.txt ../../src/txt2bin
	../../src/txt2bin test.txt test.bin

test.out: test.bin test.d9900 ../../src/dasm9900
	../../src/dasm9900 -s test.d9900 | sed '$${/^$$/d;}' > test.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d9900 ../../src/dasm9900
	../../src/dasm9900 -C cfg.d9900 > cfg.out

clean:
	rm -f test.out test.bin cfg.out cfg.bin
