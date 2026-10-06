
DIFF ?= diff -u
SHELL := /bin/bash

test: test.out test_listing.out cfg.out
	$(DIFF) test.expected test.out
	$(DIFF) test_listing.expected test_listing.out
	$(DIFF) cfg.expected cfg.out

test.out: test.hex test.dunsp ../../src/dasmunsp
	set -o pipefail; ../../src/dasmunsp -s test.dunsp | sed '$${/^$$/d;}' | sed 's/[[:space:]]*$$//' > test.out

test_listing.out: test.hex test.dunsp ../../src/dasmunsp
	set -o pipefail; ../../src/dasmunsp test.dunsp | sed 's/[[:space:]]*$$//' | sed '$${/^$$/d;}' > test_listing.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.dunsp ../../src/dasmunsp
	../../src/dasmunsp -C cfg.dunsp > cfg.out

clean:
	rm -f test.out test_listing.out cfg.bin cfg.out

.PHONY: test clean
