
DIFF ?= diff -u

test: test.out cfg.out
	$(DIFF) test.expected test.out
	$(DIFF) cfg.expected cfg.out

test.out: test.hex test.d78 ../../src/dasm78k3
	../../src/dasm78k3 -s test.d78 | sed '$${/^$$/d;}' > test.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d78 ../../src/dasm78k3
	../../src/dasm78k3 -C cfg.d78 > cfg.out

clean:
	rm -f test.out cfg.bin cfg.out
