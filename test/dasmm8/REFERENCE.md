# STM8 Instruction Set Reference

## Processor Information

**Processor:** STMicroelectronics STM8
**Architecture:** 8-bit CISC microcontroller
**Disassembler:** dasmm8

## Reference Documentation

### Primary Reference
- **Title:** STM8 CPU Programming Manual
- **Document ID:** PM0044
- **Publisher:** STMicroelectronics
- **URL:** https://www.st.com/resource/en/programming_manual/pm0044-stm8-cpu-programming-manual-stmicroelectronics.pdf

This is the definitive reference for the STM8 instruction set.

### Additional Documentation
- **STM8 Datasheet:** https://www.st.com/en/microcontrollers-microprocessors/stm8-8-bit-mcus.html
- **Application Notes:** Available from STMicroelectronics website

## Instruction Set Overview

The STM8 is a CISC processor with a rich instruction set:

### Addressing Modes
- Immediate (#$42)
- Short Direct ($10 - zero page)
- Long Direct ($1000)
- Indexed with X register (X), ($10,X), ($1000,X)
- Indexed with Y register (Y), ($10,Y), ($1000,Y)
- Indexed with SP register ($10,SP)
- Short Pointer [$10.w]
- Short Pointer Indexed ([$10.w],X), ([$10.w],Y)

### Instruction Categories
1. **System Control** - BREAK, HALT, NOP, TRAP, WFI, WFE, IRET
2. **Interrupt Management** - RIM, SIM, INT
3. **Condition Codes** - CCF, RCF, RVF, SCF
4. **Load/Store** - LD (8-bit), LDW (16-bit)
5. **Stack** - PUSH, POP, PUSHW, POPW
6. **Arithmetic** - ADD, ADC, SUB, SBC, INC, DEC, NEG, MUL, DIV
7. **Logical** - AND, OR, XOR, CPL
8. **Compare/Test** - CP, CPW, TNZ, TNZW
9. **Bit Operations** - BSET, BRES, BCPL, BTJT, BTJF
10. **Shift/Rotate** - SLL, SRL, SRA, RLC, RRC, RLWA, RRWA, SWAP
11. **Jump** - JP, JPF, JRA, JR (conditional), BTJT, BTJF
12. **Call/Return** - CALL, CALLF, CALLR, RET, RETF
13. **Exchange** - EXG, EXGW, MOV

### Register Set
- **A** - 8-bit Accumulator
- **X** - 16-bit Index Register (XH:XL)
- **Y** - 16-bit Index Register (YH:YL)
- **SP** - 16-bit Stack Pointer
- **PC** - 24-bit Program Counter
- **CC** - 8-bit Condition Code Register (flags)

### Prefix Bytes
The STM8 uses prefix bytes to extend addressing modes and access Y register:
- **0x90** - Y register access prefix
- **0x91** - Short pointer indexed with Y
- **0x92** - Short pointer or long pointer access
- **0x72** - Pre-decrement/post-increment modes (PDEC, PINC)

### Instruction Encoding
- Variable length: 1 to 5 bytes
- Most common instructions: 1-3 bytes
- Extended addressing adds 1-2 bytes
- Prefix bytes add 1 byte

## Test Coverage

The generated test suite in `generated/` includes:
- **175 instruction variants** across 15 categories
- All major addressing modes
- Representative examples from PM0044

For comprehensive testing, ensure coverage of:
- All addressing modes for each instruction
- Boundary conditions (0x00, 0xFF, 0x0000, 0xFFFF)
- Prefix byte combinations
- Conditional branches (all condition codes)

## Notes

- The STM8 supports 24-bit addressing for some instructions (JPF, CALLF)
- Some instructions have special zero-page optimization (short addressing)
- The Y register typically requires a 0x90 prefix byte
- Bit operations work on memory locations, not registers
- The SWAP instruction swaps nibbles (8-bit) or bytes (16-bit)

## See Also

- `generated/README.md` - Information about generated test files
- `test.dm8` - Manual test cases
- `../../../tools/instruction_specs/stm8.yaml` - Instruction specification used for generation
