# Makefile for generated Z80 instruction tests

DASM = ../../../src/dasmz80
TXT2BIN = ../../../src/txt2bin
DIFF ?= diff -u
SHELL := /bin/bash

.PHONY: all test golden clean help

all: test_all.bin

# Build binary from txt2bin source
test_all.bin: test_all.txt $(TXT2BIN)
	$(TXT2BIN) test_all.txt test_all.bin

# Build disassembler if needed
$(DASM):
	$(MAKE) -C ../../../src dasmz80

# Build txt2bin if needed
$(TXT2BIN):
	$(MAKE) -C ../../../src txt2bin

# Capture the full listing, including instruction addresses and bytes.
output/test_all.out: test_all.bin test_all.dz80 $(DASM)
	mkdir -p output
	set -o pipefail; $(DASM) test_all.dz80 | sed 's/[[:space:]]*$$//' | sed '$${/^$$/d;}' > $@

test: output/test_all.out
	$(DIFF) test_all.expected output/test_all.out
	python3 ../../../tools/verify_instruction_spec.py z80

# Update the reviewed snapshot after inspecting the output change.
golden: output/test_all.out
	cp output/test_all.out test_all.expected

# Clean generated files
clean:
	rm -f test_all.bin
	rm -rf output/

help:
	@echo "Targets:"
	@echo "  all    - Build test_all.bin from test_all.txt (default)"
	@echo "  test   - Compare disassembler output with reviewed snapshot"
	@echo "  golden - Update reviewed snapshot from current output"
	@echo "  clean  - Remove generated files (.bin, .out)"
