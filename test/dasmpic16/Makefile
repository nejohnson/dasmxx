
GPASM ?= gpasm
DIFF ?= diff -u

test: test.out
	$(DIFF) test.expected test.out

test.out: test.hex test.dpic16 ../../src/dasmpic16
	../../src/dasmpic16 -s test.dpic16 | sed -n '/^___CL_0001:/,$$p' > test.out

test.hex: test.asm
	$(GPASM) -q -o test.hex test.asm

clean:
	rm -f test.cod test.err test.hex test.lst test.out

