# dasmxx Test Framework

Automated testing infrastructure for the dasmxx disassembler suite.

## Overview

This framework provides:

1. **Test Runner** (`test_runner.py`) - Orchestrates test execution
2. **Test Cases** (`test_cases.py`) - Defines test structure and organization
3. **Output Verification** (`verify_output.py`) - Compares outputs against expectations

## Quick Start

```bash
# Run all tests
cd test/framework
./test_runner.py -v

# Run specific test suite
./test_runner.py path/to/test_suite.py

# Update golden files
./test_runner.py --update-golden

# Save results to JSON
./test_runner.py --output results.json
```

## Test Organization

Tests are organized into two main categories:

### 1. Tool Feature Tests (`test/tool_features/`)

Tests for dasmxx command codes and features that work across all processors:

- Code disassembly commands: `c`, `p`, `l`, `k`, `n`
- Data dump modes: `b`, `w`, `s`, `a`, `v`, `m`, `u`, `z`
- Configuration: `f`, `i`, `t`, `e`, `q`, `>`
- Output modes: `-x`, `-a`, `-s` flags

### 2. Processor Instruction Tests (`test/dasm*/`)

Comprehensive instruction set tests for each processor:

- Based on reference documentation (datasheets)
- Cover all documented instructions and addressing modes
- Test boundary cases and illegal opcodes
- Auto-generated from instruction specifications

## Writing Tests

### Basic Test Case

```python
from pathlib import Path
from test_cases import TestCase, VerificationMode

test = TestCase(
    name="Z80 basic instructions",
    processor="z80",
    command_file=Path("test.dz80"),
    output_file=Path("output/test.out"),
    golden_file=Path("golden/test.golden"),
    verification_mode=VerificationMode.EXACT
)
```

### Test Suite

```python
from test_cases import TestSuiteBuilder

builder = TestSuiteBuilder("My Tests", Path("/path/to/tests"))

builder.add_test(
    name="Test 1",
    processor="z80",
    command_file="test1.dz80",
    golden_file="golden/test1.golden"
).add_test(
    name="Test 2",
    processor="z80",
    command_file="test2.dz80",
    expected_patterns=[r"ld\s+a,", r"ret"]
)

suite = builder.build()
```

## Verification Modes

- **EXACT**: Exact string match (default)
- **IGNORE_WHITESPACE**: Normalize whitespace before comparing
- **IGNORE_ADDRESSES**: Normalize addresses and labels (for structure comparison)
- **REGEX_PATTERNS**: Match against regex patterns instead of golden file

## Output Verification

The framework provides several verification methods:

```python
from verify_output import OutputVerifier

verifier = OutputVerifier()

# Exact comparison
result = verifier.verify_output(output_file, golden_file, mode="exact")

# Check for patterns
result = verifier.check_patterns(output_text, [
    r"^[0-9A-F]{4}.*ld\s+a,",
    r"^[0-9A-F]{4}.*ret"
])

# Verify cross-references
result = verifier.verify_cross_references(output_text)

# Verify instruction format
result = verifier.verify_instruction_format(output_text, "z80")
```

## Golden Files

Golden files contain expected output for tests. They are stored in `golden/` subdirectories.

### Creating Golden Files

1. Run the test and verify output is correct
2. Run with `--update-golden` flag to save as golden file
3. Commit golden file to version control

```bash
./test_runner.py --update-golden my_test_suite.py
```

### Updating Golden Files

When intentional changes affect output:

```bash
# Update all golden files
./test_runner.py --update-golden

# Review changes before committing
git diff test/*/golden/
```

## Directory Structure

```
test/
├── framework/              # Test framework core
│   ├── test_runner.py     # Main test orchestrator
│   ├── test_cases.py      # Test case definitions
│   ├── verify_output.py   # Output verification
│   ├── README.md          # This file
│   └── golden_files/      # Shared golden files
│
├── tool_features/         # Tool feature tests
│   ├── code_commands/     # Tests for c, p, l commands
│   ├── data_dumps/        # Tests for b, w, s commands
│   ├── output_modes/      # Tests for -x, -a, -s flags
│   └── ...
│
├── dasmz80/              # Z80 processor tests
│   ├── test.dz80         # Command file
│   ├── golden/           # Golden outputs
│   ├── spec/             # Instruction specs
│   └── REFERENCE.md      # Reference documentation
│
└── dasm<proc>/           # Other processor tests
    └── ...
```

## CI Integration

The test framework is designed for CI/CD integration:

```yaml
# Example GitHub Actions workflow
- name: Build disassemblers
  run: cd src && make

- name: Run tests
  run: |
    cd test/framework
    ./test_runner.py -v --output results.json

- name: Upload results
  uses: actions/upload-artifact@v3
  with:
    name: test-results
    path: test/framework/results.json
```

## Best Practices

1. **Test Independence**: Each test should be independent and not rely on others
2. **Minimal Test Cases**: Keep test binaries small and focused
3. **Clear Names**: Use descriptive test names that indicate what is being tested
4. **Documentation**: Include REFERENCE.md for processor tests documenting the authoritative source
5. **Golden Files**: Review changes to golden files carefully before committing
6. **Patterns Over Exact**: Use patterns for tests where addresses or labels may vary

## Adding a New Processor Test Suite

1. Create test directory:
   ```bash
   mkdir test/dasm<proc>
   ```

2. Create instruction specification:
   ```bash
   # Document the reference manual
   echo "# Reference" > test/dasm<proc>/REFERENCE.md

   # Create instruction spec (or use generator)
   mkdir test/dasm<proc>/spec
   ```

3. Create test command file:
   ```
   f test.bin
   c0000
   e1000
   ```

4. Generate golden file:
   ```bash
   cd test/framework
   ./test_runner.py --update-golden
   ```

5. Add to test discovery (automatically discovered by glob pattern)

## Future Enhancements

- [ ] Parallel test execution
- [ ] Test coverage reports
- [ ] Performance benchmarking
- [ ] Visual diff tool for output comparison
- [ ] Web-based test result viewer
