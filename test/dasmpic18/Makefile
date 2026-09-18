
GPASM ?= gpasm
DIFF ?= diff -u

test: test.out
	$(DIFF) test.expected test.out

test.out: test.hex test.dpic18 ../../src/dasmpic18
	../../src/dasmpic18 -s test.dpic18 | sed -n '/^___CL_0001:/,$$p' > test.out

test.hex: test.asm
	$(GPASM) -q -o test.hex test.asm

clean:
	rm -f test.cod test.err test.hex test.lst test.out

