DIFF ?= diff -u
SHELL := /bin/bash
XC16_AS ?= xc16-as
XC16_OBJCOPY ?= xc16-objcopy

test: test.out test_listing.out cfg.out
	$(DIFF) test.expected test.out
	$(DIFF) test_listing.expected test_listing.out
	$(DIFF) cfg.expected cfg.out

test.out: test.hex test.ddspic33 ../../src/dasmdspic33
	set -o pipefail; ../../src/dasmdspic33 -s test.ddspic33 | sed -n '/^___CL_0001:/,$$p' | sed '$${/^$$/d;}' > test.out

test_listing.out: test.hex test.ddspic33 ../../src/dasmdspic33
	set -o pipefail; ../../src/dasmdspic33 test.ddspic33 | sed -n '/^___CL_0001:/,$$p' | sed 's/[[:space:]]*$$//' | sed '$${/^$$/d;}' > test_listing.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.ddspic33 ../../src/dasmdspic33
	../../src/dasmdspic33 -C cfg.ddspic33 > cfg.out

regen: test.o
	$(XC16_OBJCOPY) -O ihex test.o test.hex

test.o: test.s
	$(XC16_AS) -p33FJ64GP802 -o test.o test.s

clean:
	rm -f test.err test.lst test.o test.out test_listing.out cfg.bin cfg.out

.PHONY: test clean
