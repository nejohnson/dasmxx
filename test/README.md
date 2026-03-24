# dasmxx Test Suite

This directory contains automated tests for the dasmxx disassembler suite.

## Test Organization

### Framework (`framework/`)
Core testing infrastructure written in Python:
- `test_runner.py` - Test orchestration and execution
- `test_cases.py` - Test case definitions and builders
- `verify_output.py` - Output verification and comparison
- Complete documentation in `framework/README.md`

### Tool Feature Tests (`tool_features/`)
Tests for dasmxx command codes and features that work across all processors:
- Code disassembly commands (c, p, l, k, n)
- Data dump modes (b, w, s, a, v, m, u, z)
- Configuration commands (f, i, t, e, q, >)
- Output modes (-x, -a, -s flags)

**12 automated tests, all passing**

### Processor-Specific Tests (`dasm*/`)
Instruction set tests for each supported processor:
- Comprehensive coverage of all documented instructions
- Based on reference documentation (datasheets)
- Generated from specifications in `../tools/instruction_specs/`

Current processors with generated tests:
- Z80 (142 instructions) - pilot implementation

## Workflow: Binary Files

**IMPORTANT:** Binary test files (*.bin) are **generated from source** and should NOT be committed to version control.

### Source Files (Committed)
- `*.txt` - txt2bin source files (human-readable hex with comments)
- `*.d*` - Command files for disassembler
- `*.expected` - Expected output (reference from specifications)
- `Makefile` - Build automation

### Generated Files (Not Committed, in .gitignore)
- `*.bin` - Binary test files (built from .txt using txt2bin)
- `*.out` - Disassembler output from test runs
- `*.golden` - Verified "known good" outputs (optional)
- `output/` - Test framework output directory

### Building Test Binaries

Test binaries are automatically built from .txt source files when needed:

```bash
# In any test directory with a Makefile:
make                # Build binary from .txt
make test           # Build and run tests
make clean          # Remove generated files
```

The Makefiles will automatically:
1. Build txt2bin if needed
2. Build the disassembler if needed
3. Generate .bin from .txt
4. Run tests

## Running Tests

### Quick Test All Tool Features
```bash
cd tool_features
make test
```

### Test Specific Processor
```bash
cd dasmz80/generated
make test
```

### Using Test Framework Directly
```bash
cd tool_features
python3 run_tests.py -v
```

### Update Golden Files
```bash
cd tool_features
make golden
```

## Adding Tests

### Adding a Tool Feature Test

1. Create test binary source:
```bash
cd tool_features/testdata
cat > my_test.txt <<EOF
3E  # ld a,$42
42
C9  # ret
EOF
```

2. Create command file:
```bash
cd tool_features/code_commands
cat > test_my_feature.dz80 <<EOF
f../testdata/my_test.bin
c0000
e0010
EOF
```

3. Add to test suite in `run_tests.py`:
```python
builder.add_test(
    name="My Feature Test",
    processor="z80",
    command_file="code_commands/test_my_feature.dz80",
    golden_file="golden/test_my_feature.golden"
)
```

4. Generate golden file:
```bash
make golden
```

### Adding a Processor Test Suite

1. Create instruction specification:
```bash
cd ../tools/instruction_specs
cat > mynewproc.yaml <<EOF
processor: mynewproc
reference:
  title: "MyNewProc Manual"
  version: "1.0"
  url: "https://..."
instructions:
  - category: "Data Transfer"
    variants:
      - mnemonic: "MOV A,B"
        operands: "A,B"
        opcode: [0x78]
        reference: "page 42"
EOF
```

2. Generate test suite:
```bash
cd ../tools
./gen_instruction_tests.py mynewproc -v
```

3. Build and test:
```bash
cd ../test/dasmmynewproc/generated
make test
```

## Test Coverage

### Tool Features: ✅ Complete
- [x] Code disassembly (c, p, l commands)
- [x] Procedure labels (auto-generated)
- [x] Manual labels
- [x] Byte dumps (b command)
- [x] Word dumps (w command)
- [x] String dumps (s command)
- [x] UTF-16 dumps (u command)
- [x] Cross-reference output (-x flag)
- [x] Assembler output (-a flag)
- [x] Stripped output (-s flag)

### Processor Instructions
- [x] Z80: 142 instructions (8 categories)
- [ ] 6502: Pending
- [ ] 8051: Pending
- [ ] AVR: Pending
- [ ] STM8: Pending
- [ ] Other processors: Pending

## Continuous Integration

The test suite is designed for CI/CD integration:

```yaml
# Example GitHub Actions workflow
steps:
  - name: Build disassemblers
    run: cd src && make

  - name: Run tool feature tests
    run: cd test/tool_features && make test

  - name: Run processor tests
    run: |
      cd test/dasmz80/generated && make test
      # Add more processors as available
```

## Troubleshooting

### Binary file missing
```
Error: cannot open '../testdata/simple_code.bin'
```
**Solution:** Run `make` to build binary from .txt source

### Test failures after code changes
```
[FAIL] Some test
  Output verification failed: Output does not match expected
```
**Solution:**
1. Review changes carefully
2. If changes are intentional, update golden files: `make golden`
3. Commit updated golden files

### Disassembler not found
```
Error: Disassembler not found for z80
```
**Solution:** Build disassembler: `cd ../../src && make dasmz80`

## Best Practices

1. **Never commit .bin files** - They're generated from .txt sources
2. **Always review golden file changes** - Use `git diff` before committing
3. **Document test intent** - Add comments in test files
4. **Test one thing at a time** - Keep tests focused and small
5. **Use descriptive names** - Test names should explain what they verify

## See Also

- `framework/README.md` - Detailed test framework documentation
- `../tools/gen_instruction_tests.py` - Instruction test generator
- `../ARCHITECTURE.md` - System architecture documentation
- `../IMPLEMENTATION_STATUS.md` - Current implementation status
