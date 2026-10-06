
DIFF ?= diff -u
SHELL := /bin/bash

test: output/test.out output/trap.out cfg.out
	$(DIFF) test.expected output/test.out
	$(DIFF) trap.expected output/trap.out
	$(DIFF) cfg.expected cfg.out

test.bin: test.txt ../../src/txt2bin
	../../src/txt2bin test.txt test.bin

trap.bin: trap.txt ../../src/txt2bin
	../../src/txt2bin trap.txt trap.bin

output/test.out: test.bin test.d7000 ../../src/dasm7000
	mkdir -p output
	set -o pipefail; ../../src/dasm7000 -s test.d7000 | sed 's/[[:space:]]*$$//' | sed '$${/^$$/d;}' > output/test.out

output/trap.out: trap.bin trap.d7000 ../../src/dasm7000
	mkdir -p output
	set -o pipefail; ../../src/dasm7000 -s trap.d7000 | sed 's/[[:space:]]*$$//' | sed '$${/^$$/d;}' > output/trap.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d7000 ../../src/dasm7000
	../../src/dasm7000 -C cfg.d7000 > cfg.out

clean:
	rm -f test.bin trap.bin cfg.bin cfg.out
	rm -rf output/

.PHONY: test clean
