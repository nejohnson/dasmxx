DIFF ?= diff -u

test: test.out testx.out
	$(DIFF) test.expected test.out
	$(DIFF) testx.expected testx.out

test.bin: test.txt ../../src/txt2bin
	../../src/txt2bin test.txt test.bin

testx.bin: testx.txt ../../src/txt2bin
	../../src/txt2bin testx.txt testx.bin

test.out: test.bin test.d430 ../../src/dasm430
	../../src/dasm430 -s test.d430 | sed '$${/^$$/d;}' > test.out

testx.out: testx.bin testx.d430 ../../src/dasm430
	../../src/dasm430 -s testx.d430 | sed '$${/^$$/d;}' > testx.out

clean:
	rm -f test.out test.bin testx.out testx.bin
