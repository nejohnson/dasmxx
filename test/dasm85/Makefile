
DIFF ?= diff -u
SHELL := /bin/bash

test: output/test.out cfg.out
	$(DIFF) test.expected output/test.out
	$(DIFF) cfg.expected cfg.out

test.bin: test.txt ../../src/txt2bin
	../../src/txt2bin test.txt test.bin

output/test.out: test.bin test.d85 ../../src/dasm85
	mkdir -p output
	set -o pipefail; ../../src/dasm85 -s test.d85 | sed 's/[[:space:]]*$$//' | sed '$${/^$$/d;}' > output/test.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d85 ../../src/dasm85
	../../src/dasm85 -C cfg.d85 > cfg.out

clean:
	rm -f test.bin cfg.bin cfg.out
	rm -rf output/

.PHONY: test clean
