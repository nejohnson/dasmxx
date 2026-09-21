
DIFF ?= diff -u

test: test.out
	$(DIFF) test.expected test.out

test.out: test.hex test.dunsp ../../src/dasmunsp
	../../src/dasmunsp -s test.dunsp | sed '$${/^$$/d;}' | sed 's/[[:space:]]*$$//' > test.out

clean:
	rm -f test.out

