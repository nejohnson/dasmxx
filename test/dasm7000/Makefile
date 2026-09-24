
DIFF ?= diff -u

test: cfg.out
	../../src/txt2bin test.txt test.bin
	../../src/dasm7000 test.d7000
	$(DIFF) cfg.expected cfg.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d7000 ../../src/dasm7000
	../../src/dasm7000 -C cfg.d7000 > cfg.out

clean:
	rm -f test.bin cfg.bin cfg.out
