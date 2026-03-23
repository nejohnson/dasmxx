# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

dasmxx is a suite of disassemblers for 8-bit and 16-bit microprocessors. It provides script-driven disassembly that reads raw binary files and produces verbose listings with cross-references.

## Git Commit Guidelines

**IMPORTANT**: Do NOT add "Co-Authored-By" lines to commit messages in this project. Keep commit messages concise with just the subject line and body if needed.

## Build Commands

Build all disassemblers:
```bash
cd src && make
```

Build a specific disassembler (e.g., for AVR):
```bash
cd src && make dasmavr
```

Clean build artifacts:
```bash
cd src && make clean
```

View all available targets:
```bash
cd src && make what
```

## Testing

Run all tests:
```bash
cd test
for f in *; do pushd $f; make test; popd; done
```

Run a specific processor test (e.g., Z80):
```bash
cd test/dasmz80 && make test
```

## Architecture

### Core Framework

The disassembler suite is built on a shared framework with processor-specific modules:

- **dasmxx.c** - Main disassembler engine that:
  - Parses command list files (.dXX files)
  - Manages memory segments and output formatting
  - Handles labels, comments, and various dump modes (byte, word, string, etc.)
  - Coordinates between the generic framework and processor-specific decoders

- **xref.c** - Cross-reference tracking system that:
  - Tracks references (jumps, calls, data accesses) throughout the binary
  - Manages label generation and resolution
  - Generates cross-reference listings at end of disassembly

- **optab.c** - Opcode table handling for table-driven instruction decoding

- **dasmxx.h** - Common types and interfaces:
  - `ADDR`, `UBYTE`, `UWORD` - Platform-independent types
  - `XREF_TYPE` enum - Types of cross-references (JMP, CALL, IMM, etc.)
  - Function signatures for decoder interface

- **optab.h** - Opcode table infrastructure:
  - `optab_t` structure - Describes instruction encoding patterns
  - Macros for building opcode tables: `INSN`, `MASK`, `RANGE`, `TABLE`, etc.
  - `OPERAND_FUNC` macro for defining operand decoder functions

### Processor-Specific Decoders

Each supported processor has a `decode<proc>.c` file (e.g., `decodeavr.c`, `decodez80.c`) that:

1. Defines a `DASM_PROFILE()` with processor-specific parameters (name, max instruction length, endianness, etc.)
2. Implements `dasm_insn()` function to decode a single instruction
3. Defines opcode tables using optab.h macros
4. Implements operand functions to format instruction operands

The decoder is responsible for:
- Fetching bytes from the input file
- Matching opcodes against instruction tables
- Formatting the instruction mnemonic and operands
- Reporting cross-references via `xref_addxref()`

### Build System

Each disassembler executable is built by linking:
- Core objects: `dasmxx.o`, `xref.o`, `optab.o` (or `dasmxx.o`, `xref.o` for 8096)
- Processor-specific decoder: `decode<proc>.o`

Example for AVR: `dasmavr = dasmxx.o + xref.o + optab.o + decodeavr.o`

### Command List Files

Disassembly is controlled by command list files (.dXX extension). Key commands:

- `f<filename>` - Input binary file
- `c<XXXX>` - Mark address as code start
- `p<XXXX> [label]` - Mark procedure entry point
- `l<XXXX> [label]` - Attach label to address
- `b<XXXX>[,N]` - Byte dump
- `w<XXXX>` - Word dump
- `s<XXXX>` - String dump
- `e<XXXX>` - End of disassembly

Labels attached via 'p' and 'l' are auto-generated if not provided (PROC_nnnn, AL_nnnn).

## Adding a New Processor

1. Create `src/decode<proc>.c` with:
   - `DASM_PROFILE()` declaration
   - `dasm_insn()` implementation
   - Opcode tables using optab.h macros
   - Operand decoder functions

2. Add build target to `src/Makefile`:
   ```makefile
   D<PROC>_OBJS = ${CORE_OBJS} decode<proc>.o
   dasm<proc>: ${D<PROC>_OBJS}
       $(CC) ${D<PROC>_OBJS} -o ${@}
   ```

3. Add to `TARGETS` list in Makefile

4. Create test directory `test/dasm<proc>/` with:
   - `Makefile` (run disassembler on test files)
   - Test command file (`test.d<proc>`)
   - Test binary or text input

## Key Implementation Patterns

### Opcode Tables

Use optab.h macros to define instruction encoding:
- `INSN(mnemonic, operands, opcode, xref_type)` - Single byte match
- `MASK(mnemonic, operands, mask, value, xref_type)` - Bit pattern match
- `RANGE(mnemonic, operands, min, max, xref_type)` - Opcode range
- `TABLE(table_name, opcode)` - Jump to sub-table for multi-byte instructions

### Operand Functions

Define with `OPERAND_FUNC(name)` macro:
```c
OPERAND_FUNC(register)
{
    int reg = (opc >> 4) & 0x0F;
    operand("R%d", reg);
}
```

Use helper macros for common patterns:
- `TWO_OPERAND(a, b)` - Creates function for "a,b" operand pair
- `THREE_OPERAND(a, b, c)` - Creates function for "a,b,c" operand triple

### Cross-References

Report all address references to enable label resolution:
```c
xref_addxref(X_JMP, current_addr, target_addr);   // For jumps
xref_addxref(X_CALL, current_addr, target_addr);  // For calls
xref_addxref(X_DATA, current_addr, data_addr);    // For data references
```
