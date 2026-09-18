
DIFF ?= diff -u

test: test.out
	$(DIFF) test.expected test.out

test.out: test.hex test.d78 ../../src/dasm78k3
	../../src/dasm78k3 -s test.d78 | sed '$${/^$$/d;}' > test.out

clean:
	rm -f test.out

