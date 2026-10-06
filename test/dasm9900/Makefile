DIFF ?= diff -u
SHELL := /bin/bash

test: test.out test_listing.out cfg.out
	$(DIFF) test.expected test.out
	$(DIFF) test_listing.expected test_listing.out
	$(DIFF) cfg.expected cfg.out

test.bin: test.txt ../../src/txt2bin
	../../src/txt2bin test.txt test.bin

test.out: test.bin test.d9900 ../../src/dasm9900
	set -o pipefail; ../../src/dasm9900 -s test.d9900 | sed '$${/^$$/d;}' > test.out

test_listing.out: test.bin test.d9900 ../../src/dasm9900
	set -o pipefail; ../../src/dasm9900 test.d9900 | sed 's/[[:space:]]*$$//' | sed '$${/^$$/d;}' > test_listing.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d9900 ../../src/dasm9900
	../../src/dasm9900 -C cfg.d9900 > cfg.out

clean:
	rm -f test.out test_listing.out test.bin cfg.out cfg.bin

.PHONY: test clean
