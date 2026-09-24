DIFF ?= diff -u

test: test.out test6801.out test6800strict.out cfg.out
	$(DIFF) test.expected test.out
	$(DIFF) test6801.expected test6801.out
	$(DIFF) test6800strict.expected test6800strict.out
	$(DIFF) cfg.expected cfg.out

test.bin: test.txt ../../src/txt2bin
	../../src/txt2bin test.txt test.bin

test.out: test.bin test.d6800 ../../src/dasm6800
	../../src/dasm6800 -s test.d6800 | sed '$${/^$$/d;}' > test.out

test6801.bin: test6801.txt ../../src/txt2bin
	../../src/txt2bin test6801.txt test6801.bin

test6801.out: test6801.bin test6801.d6800 ../../src/dasm6800
	../../src/dasm6800 -s test6801.d6800 | sed '$${/^$$/d;}' > test6801.out

test6800strict.bin: test6800strict.txt ../../src/txt2bin
	../../src/txt2bin test6800strict.txt test6800strict.bin

test6800strict.out: test6800strict.bin test6800strict.d6800 ../../src/dasm6800
	../../src/dasm6800 -s test6800strict.d6800 | sed '$${/^$$/d;}' > test6800strict.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d6800 ../../src/dasm6800
	../../src/dasm6800 -C cfg.d6800 > cfg.out

clean:
	rm -f test.out test.bin test6801.out test6801.bin test6800strict.out test6800strict.bin cfg.out cfg.bin
