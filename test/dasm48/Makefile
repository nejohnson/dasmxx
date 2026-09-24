
DIFF ?= diff -u

test: cfg.out
	../../src/txt2bin test.txt test.bin
	../../src/dasm48 test.d48
	../../src/txt2bin test_page.txt test_page.bin
	../../src/dasm48 test_page.d48
	$(DIFF) cfg.expected cfg.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d48 ../../src/dasm48
	../../src/dasm48 -C cfg.d48 > cfg.out

clean:
	rm -f test.bin test_page.bin cfg.bin cfg.out
