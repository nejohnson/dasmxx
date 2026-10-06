
DIFF ?= diff -u
SHELL := /bin/bash

test: output/test.out cfg.out
	$(DIFF) test.expected output/test.out
	$(DIFF) cfg.expected cfg.out

test.bin: test.txt ../../src/txt2bin
	../../src/txt2bin test.txt test.bin

output/test.out: test.bin test.dm8 ../../src/dasmm8
	mkdir -p output
	set -o pipefail; ../../src/dasmm8 -s test.dm8 | sed 's/[[:space:]]*$$//' | sed '$${/^$$/d;}' > output/test.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.dm8 ../../src/dasmm8
	../../src/dasmm8 -C cfg.dm8 > cfg.out

clean:
	rm -f test.bin cfg.bin cfg.out
	rm -rf output/

.PHONY: test clean
