DIFF ?= diff -u

test: test.out
	$(DIFF) test.expected test.out

test.bin: test.txt ../../src/txt2bin
	../../src/txt2bin test.txt test.bin

test.out: test.bin test.dz8 ../../src/dasmz8
	../../src/dasmz8 -s test.dz8 | sed '$${/^$$/d;}' > test.out

clean:
	rm -f test.out test.bin
