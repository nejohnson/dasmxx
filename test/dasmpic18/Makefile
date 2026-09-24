
DIFF ?= diff -u
GPASM ?= gpasm

test: test.out cfg.out
	$(DIFF) test.expected test.out
	$(DIFF) cfg.expected cfg.out

test.out: test.hex test.dpic18 ../../src/dasmpic18
	../../src/dasmpic18 -s test.dpic18 | sed -n '/^___CL_0001:/,$$p' | sed '$${/^$$/d;}' > test.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.dpic18 ../../src/dasmpic18
	../../src/dasmpic18 -C cfg.dpic18 > cfg.out

regen: test.asm
	$(GPASM) -q -o test.hex test.asm

clean:
	rm -f test.cod test.err test.lst test.out cfg.bin cfg.out
