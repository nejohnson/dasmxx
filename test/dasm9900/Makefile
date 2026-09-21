DIFF ?= diff -u

test: test.out
	$(DIFF) test.expected test.out

test.bin: test.txt ../../src/txt2bin
	../../src/txt2bin test.txt test.bin

test.out: test.bin test.d9900 ../../src/dasm9900
	../../src/dasm9900 -s test.d9900 | sed '$${/^$$/d;}' > test.out

clean:
	rm -f test.out test.bin
