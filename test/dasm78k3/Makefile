
DIFF ?= diff -u
SHELL := /bin/bash

test: test.out test_listing.out cfg.out
	$(DIFF) test.expected test.out
	$(DIFF) test_listing.expected test_listing.out
	$(DIFF) cfg.expected cfg.out

test.out: test.hex test.d78 ../../src/dasm78k3
	set -o pipefail; ../../src/dasm78k3 -s test.d78 | sed '$${/^$$/d;}' > test.out

test_listing.out: test.hex test.d78 ../../src/dasm78k3
	set -o pipefail; ../../src/dasm78k3 test.d78 | sed 's/[[:space:]]*$$//' | sed '$${/^$$/d;}' > test_listing.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d78 ../../src/dasm78k3
	../../src/dasm78k3 -C cfg.d78 > cfg.out

clean:
	rm -f test.out test_listing.out cfg.bin cfg.out

.PHONY: test clean
