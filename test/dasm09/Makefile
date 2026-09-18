DIFF ?= diff -u

test: test.out
	$(DIFF) test.expected test.out

test.out: test.hex test.d09 ../../src/dasm09
	../../src/dasm09 -s test.d09 | sed '$${/^$$/d;}' > test.out

clean:
	rm -f test.out
