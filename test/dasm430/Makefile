DIFF ?= diff -u

test: test.out
	$(DIFF) test.expected test.out

test.bin: test.txt ../../src/txt2bin
	../../src/txt2bin test.txt test.bin

test.out: test.bin test.d430 ../../src/dasm430
	../../src/dasm430 -s test.d430 | sed '$${/^$$/d;}' > test.out

clean:
	rm -f test.out test.bin
