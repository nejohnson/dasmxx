
DIFF ?= diff -u

test: cfg.out
	../../src/txt2bin test.txt test.bin
	../../src/dasm1802 test.d1802
	$(DIFF) cfg.expected cfg.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d1802 ../../src/dasm1802
	../../src/dasm1802 -C cfg.d1802 > cfg.out

clean:
	rm -f test.bin cfg.bin cfg.out
