DIFF ?= diff -u

test: test.out test6801.out test6800strict.out
	$(DIFF) test.expected test.out
	$(DIFF) test6801.expected test6801.out
	$(DIFF) test6800strict.expected test6800strict.out

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

clean:
	rm -f test.out test.bin test6801.out test6801.bin test6800strict.out test6800strict.bin
