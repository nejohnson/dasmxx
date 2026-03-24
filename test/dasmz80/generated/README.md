# Z80 Instruction Tests

Comprehensive instruction set tests generated from specifications.

## Reference Documentation

- **Document**: Z80 CPU User Manual
- **Version**: UM008008-0116
- **URL**: https://www.zilog.com/docs/z80/um0080.pdf

## Test Statistics

- Total instructions: 142
- Categories: 8

### Instructions by Category

- 16-bit Arithmetic: 12 instructions
- 16-bit Load: 15 instructions
- 8-bit Arithmetic: 28 instructions
- 8-bit Load: 42 instructions
- Bit Operations: 6 instructions
- CPU Control: 7 instructions
- Jump, Call, Return: 21 instructions
- Rotate and Shift: 11 instructions

## Files

**Source files (committed to version control):**
- `test_all.txt` - txt2bin source with all instructions (source of truth)
- `test_all.dz80` - Command file for disassembler
- `test_all.expected` - Expected disassembly output (reference from spec)
- `Makefile` - Build and test automation

**Generated files (not committed, built from .txt):**
- `test_all.bin` - Binary test file (built from test_all.txt)
- `test_all.out` - Disassembler output from test run
- `test_all.golden` - Verified output from disassembler (optional)

## Building and Running Tests

### Quick Start
```bash
# Build binary and run test (one command)
make test

# Compare output with expected
diff test_all.expected test_all.out
```

### Step by Step
```bash
# 1. Build binary from source
make test_all.bin
# This runs: txt2bin test_all.txt test_all.bin

# 2. Run disassembler
make test

# 3. Compare with expected output
diff test_all.expected test_all.out
```

### Create Golden File
```bash
# After verifying output is correct, create golden file
make golden
```

## Regenerating Tests

```bash
cd ../../tools
./gen_instruction_tests.py z80
```
