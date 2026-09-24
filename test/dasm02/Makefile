DIFF ?= diff -u

test: test.out test65c02.out test6502strict.out test65816.out test65816wide.out cfg.out cfg65816.out
	$(DIFF) test.expected test.out
	$(DIFF) test65c02.expected test65c02.out
	$(DIFF) test6502strict.expected test6502strict.out
	$(DIFF) test65816.expected test65816.out
	$(DIFF) test65816wide.expected test65816wide.out
	$(DIFF) cfg.expected cfg.out
	$(DIFF) cfg65816.expected cfg65816.out

test.out: test.hex test.d02 ../../src/dasm02
	../../src/dasm02 -s test.d02 | sed '$${/^$$/d;}' > test.out

test65c02.bin: test65c02.txt ../../src/txt2bin
	../../src/txt2bin test65c02.txt test65c02.bin

test65c02.out: test65c02.bin test65c02.d02 ../../src/dasm02
	../../src/dasm02 -s test65c02.d02 | sed '$${/^$$/d;}' > test65c02.out

test6502strict.bin: test6502strict.txt ../../src/txt2bin
	../../src/txt2bin test6502strict.txt test6502strict.bin

test6502strict.out: test6502strict.bin test6502strict.d02 ../../src/dasm02
	../../src/dasm02 -s test6502strict.d02 | sed '$${/^$$/d;}' > test6502strict.out

test65816.bin: test65816.txt ../../src/txt2bin
	../../src/txt2bin test65816.txt test65816.bin

test65816.out: test65816.bin test65816.d02 ../../src/dasm02
	../../src/dasm02 -s test65816.d02 | sed '$${/^$$/d;}' > test65816.out

test65816wide.bin: test65816wide.txt ../../src/txt2bin
	../../src/txt2bin test65816wide.txt test65816wide.bin

test65816wide.out: test65816wide.bin test65816wide.d02 ../../src/dasm02
	../../src/dasm02 -s test65816wide.d02 | sed '$${/^$$/d;}' > test65816wide.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d02 ../../src/dasm02
	../../src/dasm02 -C cfg.d02 > cfg.out

cfg65816.bin: cfg65816.txt ../../src/txt2bin
	../../src/txt2bin cfg65816.txt cfg65816.bin

cfg65816.out: cfg65816.bin cfg65816.d02 ../../src/dasm02
	../../src/dasm02 -C cfg65816.d02 > cfg65816.out

clean:
	rm -f test.out test65c02.out test65c02.bin test6502strict.out test6502strict.bin test65816.out test65816.bin test65816wide.out test65816wide.bin cfg.out cfg.bin cfg65816.out cfg65816.bin
