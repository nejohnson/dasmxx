
DIFF ?= diff -u
SHELL := /bin/bash

test: output/test.out cfg.out
	$(DIFF) test.expected output/test.out
	$(DIFF) cfg.expected cfg.out

test.bin: test.txt ../../src/txt2bin
	../../src/txt2bin test.txt test.bin

output/test.out: test.bin test.d1802 ../../src/dasm1802
	mkdir -p output
	set -o pipefail; ../../src/dasm1802 -s test.d1802 | sed 's/[[:space:]]*$$//' | sed '$${/^$$/d;}' > output/test.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d1802 ../../src/dasm1802
	../../src/dasm1802 -C cfg.d1802 > cfg.out

clean:
	rm -f test.bin cfg.bin cfg.out
	rm -rf output/

.PHONY: test clean
