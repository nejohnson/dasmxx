# Makefile for tool features tests

TXT2BIN = ../../src/txt2bin
TESTDATA = testdata/simple_code.bin testdata/rom_low.bin testdata/rom_high.bin

.PHONY: all test clean

all: $(TESTDATA)

# Build test binary from txt2bin source
%.bin: %.txt $(TXT2BIN)
	$(TXT2BIN) $< $@

# Build txt2bin if needed
$(TXT2BIN):
	$(MAKE) -C ../../src txt2bin

# Run all tests
test: $(TESTDATA)
	python3 run_tests.py -v
	@mkdir -p output
	@cd input_formats && if ../../../src/dasmz80 test_bad_ihex.dz80 > ../output/test_bad_ihex.out 2>&1; then echo "Expected bad Intel HEX checksum to fail"; exit 1; fi
	@grep -q "Bad Intel HEX checksum" output/test_bad_ihex.out
	@cd input_formats && if ../../../src/dasmz80 test_bad_srec.dz80 > ../output/test_bad_srec.out 2>&1; then echo "Expected bad Motorola S-record checksum to fail"; exit 1; fi
	@grep -q "Bad Motorola S-record checksum" output/test_bad_srec.out
	@cd input_formats && if ../../../src/dasmz80 test_ihex_offset.dz80 > ../output/test_ihex_offset.out 2>&1; then echo "Expected Intel HEX file offset to fail"; exit 1; fi
	@grep -q "File offset command is only supported for binary input" output/test_ihex_offset.out
	@cd input_formats && if ../../../src/dasmz80 test_overlap.dz80 > ../output/test_overlap.out 2>&1; then echo "Expected overlapping mapped ROMs to fail"; exit 1; fi
	@grep -q "Input record overlaps existing data" output/test_overlap.out

# Update golden files
golden: $(TESTDATA)
	python3 run_tests.py -v --update-golden

# Clean generated files
clean:
	rm -f $(TESTDATA)
	rm -rf output/

.PHONY: help
help:
	@echo "Targets:"
	@echo "  all    - Build test binary from .txt source (default)"
	@echo "  test   - Run all tool feature tests"
	@echo "  golden - Update golden files"
	@echo "  clean  - Remove generated files"
