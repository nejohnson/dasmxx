
DIFF ?= diff -u

test: test.out
	$(DIFF) test.expected test.out

test.out: test.hex test.d96 ../../src/dasm96
	../../src/dasm96 -s test.d96 | sed '$${/^$$/d;}' | sed 's/[[:space:]]*$$//' > test.out

clean:
	rm -f test.out
