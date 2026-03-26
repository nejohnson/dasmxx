# Makefile for generated STM8 instruction tests

DASM = ../../../src/dasmm8
TXT2BIN = ../../../src/txt2bin

.PHONY: all test golden clean

all: test_all.bin

# Build binary from txt2bin source
test_all.bin: test_all.txt $(TXT2BIN)
	$(TXT2BIN) test_all.txt test_all.bin

# Build disassembler if needed
$(DASM):
	$(MAKE) -C ../../../src dasmm8

# Build txt2bin if needed
$(TXT2BIN):
	$(MAKE) -C ../../../src txt2bin

# Run test and compare with expected output
test: test_all.bin $(DASM)
	$(DASM) test_all.dm8 > test_all.out
	@echo "Test completed. Output saved to test_all.out"
	@echo "To compare with expected: diff test_all.expected test_all.out"

# Create/update golden file from current disassembler output
golden: test_all.bin $(DASM)
	$(DASM) test_all.dm8 > test_all.golden
	@echo "Golden file updated: test_all.golden"

# Clean generated files
clean:
	rm -f test_all.bin test_all.out

.PHONY: help
help:
	@echo "Targets:"
	@echo "  all    - Build test_all.bin from test_all.txt (default)"
	@echo "  test   - Run disassembler and generate output"
	@echo "  golden - Create/update golden file from current output"
	@echo "  clean  - Remove generated files (.bin, .out)"
