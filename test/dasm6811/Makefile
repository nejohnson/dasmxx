DIFF ?= diff -u

test: test.out cfg.out
	$(DIFF) test.expected test.out
	$(DIFF) cfg.expected cfg.out

test.bin: test.txt ../../src/txt2bin
	../../src/txt2bin test.txt test.bin

test.out: test.bin test.d6811 ../../src/dasm6811
	../../src/dasm6811 -s test.d6811 | sed '$${/^$$/d;}' > test.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d6811 ../../src/dasm6811
	../../src/dasm6811 -C cfg.d6811 > cfg.out

clean:
	rm -f test.out test.bin cfg.out cfg.bin
