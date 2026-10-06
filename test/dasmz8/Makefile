DIFF ?= diff -u
SHELL := /bin/bash

test: test.out test_listing.out cfg.out
	$(DIFF) test.expected test.out
	$(DIFF) test_listing.expected test_listing.out
	$(DIFF) cfg.expected cfg.out

test.bin: test.txt ../../src/txt2bin
	../../src/txt2bin test.txt test.bin

test.out: test.bin test.dz8 ../../src/dasmz8
	set -o pipefail; ../../src/dasmz8 -s test.dz8 | sed '$${/^$$/d;}' > test.out

test_listing.out: test.bin test.dz8 ../../src/dasmz8
	set -o pipefail; ../../src/dasmz8 test.dz8 | sed 's/[[:space:]]*$$//' | sed '$${/^$$/d;}' > test_listing.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.dz8 ../../src/dasmz8
	../../src/dasmz8 -C cfg.dz8 > cfg.out

clean:
	rm -f test.out test_listing.out test.bin cfg.out cfg.bin

.PHONY: test clean
