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

- `test_all.txt` - txt2bin source with all instructions
- `test_all.bin` - Binary test file
- `test_all.dz80` - Command file
- `test_all.reference` - YAML intent checked by `make test`
- `test_all.expected` - Reviewed disassembler output used by `make test`

## Running Tests

```bash
# Build inputs, run the disassembler, and compare its output
make test
```

To accept an intentional output change, run `make golden` and review the diff.
`make test` also checks the YAML opcodes and instruction meanings.
The verifier accepts equivalent syntax for implicit operands,
hex number width, branch aliases, and resolved relative targets.

## Regenerating Tests

```bash
cd ../../../tools
./gen_instruction_tests.py z80
```
