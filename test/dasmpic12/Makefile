
DIFF ?= diff -u
GPASM ?= gpasm

test: test.out
	$(DIFF) test.expected test.out

test.out: test.hex test.dpic12 ../../src/dasmpic12
	../../src/dasmpic12 -s test.dpic12 | sed -n '/^___CL_0001:/,$$p' > test.out

regen: test.asm
	$(GPASM) -q -o test.hex test.asm

clean:
	rm -f test.cod test.err test.lst test.out
