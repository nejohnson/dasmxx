DIFF ?= diff -u
SHELL := /bin/bash

test: test.out test_listing.out cfg.out
	$(DIFF) test.expected test.out
	$(DIFF) test_listing.expected test_listing.out
	$(DIFF) cfg.expected cfg.out

test.bin: test.txt ../../src/txt2bin
	../../src/txt2bin test.txt test.bin

test.out: test.bin test.d6811 ../../src/dasm6811
	set -o pipefail; ../../src/dasm6811 -s test.d6811 | sed '$${/^$$/d;}' > test.out

test_listing.out: test.bin test.d6811 ../../src/dasm6811
	set -o pipefail; ../../src/dasm6811 test.d6811 | sed 's/[[:space:]]*$$//' | sed '$${/^$$/d;}' > test_listing.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d6811 ../../src/dasm6811
	../../src/dasm6811 -C cfg.d6811 > cfg.out

clean:
	rm -f test.out test_listing.out test.bin cfg.out cfg.bin

.PHONY: test clean
