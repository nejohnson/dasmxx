# Makefile for tool features tests

TXT2BIN = ../../src/txt2bin
TESTDATA = testdata/simple_code

.PHONY: all test clean

all: $(TESTDATA).bin

# Build test binary from txt2bin source
$(TESTDATA).bin: $(TESTDATA).txt $(TXT2BIN)
	$(TXT2BIN) $(TESTDATA).txt $(TESTDATA).bin

# Build txt2bin if needed
$(TXT2BIN):
	$(MAKE) -C ../../src txt2bin

# Run all tests
test: $(TESTDATA).bin
	python3 run_tests.py -v

# Update golden files
golden: $(TESTDATA).bin
	python3 run_tests.py -v --update-golden

# Clean generated files
clean:
	rm -f $(TESTDATA).bin
	rm -rf output/

.PHONY: help
help:
	@echo "Targets:"
	@echo "  all    - Build test binary from .txt source (default)"
	@echo "  test   - Run all tool feature tests"
	@echo "  golden - Update golden files"
	@echo "  clean  - Remove generated files"
