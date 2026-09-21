DIFF ?= diff -u

test: test.out test65c02.out test6502strict.out
	$(DIFF) test.expected test.out
	$(DIFF) test65c02.expected test65c02.out
	$(DIFF) test6502strict.expected test6502strict.out

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

clean:
	rm -f test.out test65c02.out test65c02.bin test6502strict.out test6502strict.bin
