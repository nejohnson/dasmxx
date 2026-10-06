DIFF ?= diff -u
SHELL := /bin/bash

test: test.out output/legacy.out cfg.out
	$(DIFF) test.expected test.out
	$(DIFF) legacy.expected output/legacy.out
	$(DIFF) cfg.expected cfg.out

test.out: test.hex test.d51 ../../src/dasm51
	set -o pipefail; ../../src/dasm51 -s test.d51 | sed '$${/^$$/d;}' > test.out

output/legacy.out: test.bin legacy.d51 8051regs.inc ../../src/dasm51
	mkdir -p output
	set -o pipefail; ../../src/dasm51 -s legacy.d51 | sed 's/[[:space:]]*$$//' | sed '$${/^$$/d;}' > output/legacy.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d51 ../../src/dasm51
	../../src/dasm51 -C cfg.d51 > cfg.out

legacy: output/legacy.out
	$(DIFF) legacy.expected output/legacy.out

clean:
	rm -f test.out cfg.out cfg.bin
	rm -rf output/

.PHONY: test legacy clean
