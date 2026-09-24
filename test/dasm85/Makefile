
DIFF ?= diff -u

test: test.bin cfg.out
	../../src/dasm85 test.d85
	$(DIFF) cfg.expected cfg.out

test.bin: test.txt ../../src/txt2bin
	../../src/txt2bin test.txt test.bin

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d85 ../../src/dasm85
	../../src/dasm85 -C cfg.d85 > cfg.out

clean:
	rm -f test.bin cfg.bin cfg.out
