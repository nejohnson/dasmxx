
DIFF ?= diff -u
SHELL := /bin/bash
GPASM ?= gpasm

test: test.out test_listing.out cfg.out
	$(DIFF) test.expected test.out
	$(DIFF) test_listing.expected test_listing.out
	$(DIFF) cfg.expected cfg.out

test.out: test.hex test.dpic18 ../../src/dasmpic18
	set -o pipefail; ../../src/dasmpic18 -s test.dpic18 | sed -n '/^___CL_0001:/,$$p' | sed '$${/^$$/d;}' > test.out

test_listing.out: test.hex test.dpic18 ../../src/dasmpic18
	set -o pipefail; ../../src/dasmpic18 test.dpic18 | sed -n '/^___CL_0001:/,$$p' | sed 's/[[:space:]]*$$//' | sed '$${/^$$/d;}' > test_listing.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.dpic18 ../../src/dasmpic18
	../../src/dasmpic18 -C cfg.dpic18 > cfg.out

regen: test.asm
	$(GPASM) -q -o test.hex test.asm

clean:
	rm -f test.cod test.err test.lst test.out test_listing.out cfg.bin cfg.out

.PHONY: test clean
