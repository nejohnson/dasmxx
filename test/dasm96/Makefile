
DIFF ?= diff -u
SHELL := /bin/bash

test: test.out test_listing.out cfg.out
	$(DIFF) test.expected test.out
	$(DIFF) test_listing.expected test_listing.out
	$(DIFF) cfg.expected cfg.out

test.out: test.hex test.d96 ../../src/dasm96
	set -o pipefail; ../../src/dasm96 -s test.d96 | sed '$${/^$$/d;}' | sed 's/[[:space:]]*$$//' > test.out

test_listing.out: test.hex test.d96 ../../src/dasm96
	set -o pipefail; ../../src/dasm96 test.d96 | sed 's/[[:space:]]*$$//' | sed '$${/^$$/d;}' > test_listing.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d96 ../../src/dasm96
	../../src/dasm96 -C cfg.d96 > cfg.out

clean:
	rm -f test.out test_listing.out cfg.bin cfg.out

.PHONY: test clean
