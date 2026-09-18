DIFF ?= diff -u

test: test.out
	$(DIFF) test.expected test.out

test.out: test.hex test.d51 ../../src/dasm51
	../../src/dasm51 -s test.d51 | sed '$${/^$$/d;}' > test.out

legacy:
	../../src/dasm51 legacy.d51

clean:
	rm -f test.out
