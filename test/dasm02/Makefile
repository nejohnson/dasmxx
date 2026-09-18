DIFF ?= diff -u

test: test.out
	$(DIFF) test.expected test.out

test.out: test.hex test.d02 ../../src/dasm02
	../../src/dasm02 -s test.d02 | sed '$${/^$$/d;}' > test.out

clean:
	rm -f test.out
