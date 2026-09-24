
DIFF ?= diff -u
GPASM ?= gpasm

test: test.out cfg.out
	$(DIFF) test.expected test.out
	$(DIFF) cfg.expected cfg.out

test.out: test.hex test.dpic12 ../../src/dasmpic12
	../../src/dasmpic12 -s test.dpic12 | sed -n '/^___CL_0001:/,$$p' > test.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.dpic12 ../../src/dasmpic12
	../../src/dasmpic12 -C cfg.dpic12 > cfg.out

regen: test.asm
	$(GPASM) -q -o test.hex test.asm

clean:
	rm -f test.cod test.err test.lst test.out cfg.bin cfg.out
