# M8 Instruction Tests

Comprehensive instruction set tests generated from specifications.

## Reference Documentation

- **Document**: STM8 CPU Programming Manual
- **Version**: PM0044
- **URL**: https://www.st.com/resource/en/programming_manual/pm0044-stm8-cpu-programming-manual-stmicroelectronics.pdf

## Test Statistics

- Total instructions: 175
- Categories: 15

### Instructions by Category

- 16-bit Load: 11 instructions
- Arithmetic: 30 instructions
- Bit Operations: 7 instructions
- Call and Return: 6 instructions
- Compare and Test: 10 instructions
- Condition Code: 4 instructions
- Conditional Jump: 18 instructions
- Exchange: 6 instructions
- Interrupt Management: 3 instructions
- Load and Store: 28 instructions
- Logical: 10 instructions
- Shift and Rotate: 20 instructions
- Stack Operations: 9 instructions
- System Control: 7 instructions
- Unconditional Jump: 6 instructions

## Files

- `test_all.txt` - txt2bin source with all instructions
- `test_all.bin` - Binary test file
- `test_all.dm8` - Command file
- `test_all.expected` - Expected disassembly output
- `test_all.golden` - Verified output from disassembler (run `make golden`)

## Running Tests

```bash
# Build disassembler
cd ../../src && make dasmm8

# Run test
../../src/dasmm8 test_all.dm8 > output.txt

# Compare with expected
diff test_all.expected output.txt
```

## Regenerating Tests

```bash
cd ../../tools
./gen_instruction_tests.py m8
```
