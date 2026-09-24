# Makefile for tool features tests

TXT2BIN = ../../src/txt2bin
TESTDATA = testdata/simple_code.bin testdata/rom_low.bin testdata/rom_high.bin testdata/cfg_z80.bin testdata/cfg_stress_vectors.bin testdata/cfg_stress_bank.bin

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
	@cd code_commands && ../../../src/dasmz80 -g dot -G ../output/test_cfg_dot.out test_cfg_z80.dz80 > ../output/test_cfg_dot_listing.out
	@diff -u golden/test_cfg_dot.golden output/test_cfg_dot.out
	@cd code_commands && rm -f test_cfg_z80.dot && ../../../src/dasmz80 -g dot test_cfg_z80.dz80 > ../output/test_cfg_dot_default_listing.out
	@diff -u golden/test_cfg_dot.golden code_commands/test_cfg_z80.dot
	@rm -f code_commands/test_cfg_z80.dot
	@cd code_commands && ../../../src/dasmz80 -g cmd -G ../output/test_cfg_cmd.out test_cfg_z80.dz80 > ../output/test_cfg_cmd_listing.out
	@diff -u golden/test_cfg_cmd.golden output/test_cfg_cmd.out
	@cd code_commands && ../../../src/dasmz80 -g json -G ../output/test_cfg_json.out test_cfg_z80.dz80 > ../output/test_cfg_json_listing.out
	@diff -u golden/test_cfg_json.golden output/test_cfg_json.out
	@cd code_commands && ../../../src/dasmz80 -g json -G ../output/test_cfg_stress_json.out test_cfg_stress.dz80 > ../output/test_cfg_stress_listing.out
	@diff -u golden/test_cfg_stress_json.golden output/test_cfg_stress_json.out
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
	rm -f code_commands/test_cfg_z80.dot code_commands/test_cfg_z80.json code_commands/test_cfg_z80.cfg.dz80
	rm -rf output/

.PHONY: help
help:
	@echo "Targets:"
	@echo "  all    - Build test binary from .txt source (default)"
	@echo "  test   - Run all tool feature tests"
	@echo "  golden - Update golden files"
	@echo "  clean  - Remove generated files"
