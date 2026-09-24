
DIFF ?= diff -u

test: cfg.out
	../../src/txt2bin test.txt test.bin
	../../src/dasmm8 test.dm8
	$(DIFF) cfg.expected cfg.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.dm8 ../../src/dasmm8
	../../src/dasmm8 -C cfg.dm8 > cfg.out

clean:
	rm -f test.bin cfg.bin cfg.out
