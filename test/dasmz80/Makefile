
DIFF ?= diff -u
SHELL := /bin/bash

test: output/test.out testz180.out testz80strict.out cfg.out
	$(DIFF) test.expected output/test.out
	$(DIFF) testz180.expected testz180.out
	$(DIFF) testz80strict.expected testz80strict.out
	$(DIFF) cfg.expected cfg.out

test.bin: test.txt ../../src/txt2bin
	../../src/txt2bin test.txt test.bin

output/test.out: test.bin test.dz80 ../../src/dasmz80
	mkdir -p output
	set -o pipefail; ../../src/dasmz80 -s test.dz80 | sed 's/[[:space:]]*$$//' | sed '$${/^$$/d;}' > output/test.out

testz180.bin: testz180.txt ../../src/txt2bin
	../../src/txt2bin testz180.txt testz180.bin

testz180.out: testz180.bin testz180.dz80 ../../src/dasmz80
	set -o pipefail; ../../src/dasmz80 -s testz180.dz80 | sed '$${/^$$/d;}' > testz180.out

testz80strict.bin: testz80strict.txt ../../src/txt2bin
	../../src/txt2bin testz80strict.txt testz80strict.bin

testz80strict.out: testz80strict.bin testz80strict.dz80 ../../src/dasmz80
	set -o pipefail; ../../src/dasmz80 -s testz80strict.dz80 | sed '$${/^$$/d;}' > testz80strict.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.dz80 ../../src/dasmz80
	../../src/dasmz80 -C cfg.dz80 > cfg.out

clean:
	rm -f test.bin testz180.bin testz180.out testz80strict.bin testz80strict.out cfg.bin cfg.out
	rm -rf output/

.PHONY: test clean
