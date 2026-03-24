# dasmxx Architecture Documentation

This document describes the internal architecture of the dasmxx disassembler suite.

## Table of Contents

1. [Overview](#overview)
2. [Core Components](#core-components)
3. [Decoder Interface](#decoder-interface)
4. [Memory Management](#memory-management)
5. [Cross-Reference System](#cross-reference-system)
6. [Error Handling](#error-handling)
7. [Build System](#build-system)
8. [Adding a New Processor](#adding-a-new-processor)

## Overview

dasmxx is a suite of disassemblers for 8-bit and 16-bit microprocessors. It uses a **shared core framework** with **processor-specific decoders** to provide script-driven disassembly.

### Design Philosophy

- **Separation of concerns:** Generic framework (dasmxx.c) handles I/O, formatting, and labels. Processor-specific decoders handle instruction decoding.
- **Table-driven decoding:** Most processors use optab.h for declarative instruction tables (except dasm96 which uses code-driven approach).
- **Script-driven operation:** Command files (.dXX) control disassembly, allowing fine-grained control over code/data regions.
- **Cross-reference tracking:** All address references are tracked and reported.

### Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│ User Command File (.dXX)                                    │
│ - File paths, start addresses, labels, etc.                 │
└──────────────┬──────────────────────────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────────────────────────┐
│ dasmxx.c (Main Engine)                                       │
│ - Parses command file                                        │
│ - Manages memory segments                                    │
│ - Coordinates output formatting                              │
│ - Calls decoder for instructions                             │
└──────────────┬──────────────────────────────────────────────┘
               │
               ├──────────┐
               │          │
               ▼          ▼
┌──────────────────┐   ┌──────────────────┐
│ xref.c           │   │ decode<proc>.c   │
│ Cross-reference  │   │ Processor        │
│ tracking         │   │ decoder          │
│                  │   │                  │
│ - Track refs     │   │ - dasm_insn()    │
│ - Generate       │   │ - Opcode tables  │
│   labels         │   │ - Operand fns    │
│ - Print xrefs    │   │                  │
└──────────────────┘   └────────┬─────────┘
                                │
                                ▼
                       ┌──────────────────┐
                       │ optab.c          │
                       │ Opcode table     │
                       │ matching         │
                       └──────────────────┘
```

## Core Components

### dasmxx.c - Main Disassembler Engine

**Responsibilities:**
- Parse command list files (.dXX format)
- Load binary files into memory
- Manage disassembly regions (code, data, gaps)
- Call processor decoder for instructions
- Format and output disassembly listings
- Handle various dump modes (byte, word, string, etc.)

**Key Functions:**
- `main()` - Entry point, parses command file
- `process_code()` - Disassemble code regions
- `dump_*()` - Various data dump modes (bytes, words, strings)
- `cmdlist_*()` - Command file parsing functions

**Global State:**
- `fname` - Current input filename
- `fp` - File pointer
- `pc` - Program counter
- `offset` - File offset
- `endian` - Byte order (little/big)

### xref.c - Cross-Reference System

**Responsibilities:**
- Track all address references (jumps, calls, data accesses)
- Generate automatic labels for unlabeled addresses
- Maintain reference counts
- Generate cross-reference listings

**Key Functions:**
- `xref_init()` - Initialize cross-reference system
- `xref_addxref(type, from, to)` - Record a reference
- `xref_findlabel(addr)` - Look up label for address
- `xref_addlabel(addr, name)` - Add/update label
- `xref_print()` - Print cross-reference report

**Data Structures:**
```c
typedef struct xref {
    ADDR addr;              // Target address
    char *label;            // Label name (if any)
    int refcnt;             // Reference count
    UBYTE type;             // Type: JMP, CALL, IMM, DATA, etc.
    struct xref *next;      // Linked list
} xref_t;
```

### optab.c/optab.h - Opcode Table System

**Responsibilities:**
- Match opcodes against instruction patterns
- Support multi-byte instructions
- Handle instruction variants (masks, ranges)
- Navigate instruction table trees

**Key Macros:**
```c
INSN(mnem, ops, value, xref)        // Single byte match
MASK(mnem, ops, mask, value, xref)  // Bit pattern match
RANGE(mnem, ops, lo, hi, xref)      // Range of opcodes
TABLE(name, byte)                   // Jump to sub-table
```

**Operand Functions:**
```c
OPERAND_FUNC(name) { ... }          // Define operand decoder
```

### decode<proc>.c - Processor Decoder

**Responsibilities:**
- Decode a single instruction
- Fetch bytes from input
- Format instruction mnemonic and operands
- Report cross-references
- Return instruction length

**Required Interface:**
```c
// Processor profile (must be defined)
DASM_PROFILE(
    "procname",     // Processor name
    max_insn_len,   // Maximum instruction length
    LITTLE_ENDIAN,  // Byte order
    comment_char    // Comment character
);

// Main decode function
int dasm_insn(ADDR pc, char *mnem, char *operand)
{
    // 1. Fetch opcode
    // 2. Match against instruction table
    // 3. Decode operands
    // 4. Report cross-references
    // 5. Format output
    // 6. Return instruction length
}
```

## Decoder Interface

### Contract Between dasmxx.c and Decoder

The decoder must implement:

1. **DASM_PROFILE()** macro defining:
   - Processor name (for identification)
   - Maximum instruction length (for buffer sizing)
   - Endianness (LITTLE_ENDIAN or BIG_ENDIAN)
   - Comment character (for output formatting)

2. **dasm_insn(pc, mnem, operand)** function:
   - **Input:** `pc` (program counter/address)
   - **Output:** `mnem` (mnemonic buffer, typically 16 bytes)
   - **Output:** `operand` (operand buffer, typically 128 bytes)
   - **Return:** Instruction length in bytes, or 0 for end-of-file

### Global Variables Available to Decoder

From dasmxx.c:
```c
extern FILE *fp;          // Input file pointer
extern ADDR pc;           // Current program counter
extern int offset;        // File offset
extern char *fname;       // Current filename
extern int endian;        // Byte order
```

From xref.c:
```c
// Add cross-reference
void xref_addxref(XREF_TYPE type, ADDR from, ADDR to);

// Cross-reference types
#define X_JMP   1    // Jump/branch
#define X_CALL  2    // Call/subroutine
#define X_IMM   3    // Immediate data reference
#define X_DATA  4    // Data memory reference
```

### Helper Functions

Decoders should use these functions from dasmxx.c:

```c
// Fetch a byte (advances file position)
int nextbyte(void);

// Peek at a byte (doesn't advance)
int peekbyte(void);

// Fetch multi-byte value (respects endianness)
UWORD nextword(void);

// Format output
void mnem(char *fmt, ...);       // Set mnemonic
void operand(char *fmt, ...);    // Set operand
void comment(char *fmt, ...);    // Add comment
```

### Operand Formatting Conventions

Standard operand formats used across processors:

- **Immediate:** `#$42`, `#$1234`
- **Absolute:** `$1234`
- **Register:** `A`, `BC`, `R0`
- **Indirect:** `(HL)`, `(BC)`, `@R0`
- **Indexed:** `(IX+$05)`, `($1234,X)`
- **Relative:** `$+5`, `$-3` (for PC-relative branches)

## Memory Management

### Ownership Rules

1. **dasmxx.c owns:**
   - Input file handles
   - Output buffers (mnem, operand)
   - Command list structures

2. **xref.c owns:**
   - Cross-reference list
   - Label strings (allocated via `dupstr()`)

3. **Decoder owns:**
   - Opcode tables (static data)
   - Temporary decode state

### Allocation Functions

```c
// String duplication (allocates memory)
char *dupstr(const char *s);

// Zero-initialized allocation
void *zalloc(size_t size);

// Standard free
void free(void *ptr);
```

### Memory Lifetime

- **Command list:** Allocated during parsing, freed at program exit
- **Cross-references:** Allocated during disassembly, freed before exit
- **Labels:** Allocated via `dupstr()`, owned by xref system
- **Buffers:** Static or automatic storage (mnem, operand buffers)

### Buffer Sizes

Standard buffer sizes used throughout:

```c
#define MNEM_SIZE    16     // Mnemonic buffer
#define OPERAND_SIZE 128    // Operand buffer
#define LABEL_SIZE   64     // Label name
#define LINE_SIZE    256    // Output line
```

## Cross-Reference System

### Purpose

The cross-reference system tracks all address references to:
1. Generate labels for referenced addresses
2. Provide cross-reference listings showing where each address is used
3. Help users understand code flow and data relationships

### Reference Types

```c
typedef enum {
    X_JMP = 1,   // Jump/branch instruction
    X_CALL,      // Subroutine call
    X_IMM,       // Immediate operand (address constant)
    X_DATA,      // Data memory reference
} XREF_TYPE;
```

### Usage Pattern

In decoder:
```c
int dasm_insn(ADDR pc, char *mnem, char *operand)
{
    unsigned char opcode = nextbyte();

    if (opcode == 0xC3) {  // JP nn
        ADDR target = nextword();
        xref_addxref(X_JMP, pc, target);  // Record jump reference
        mnem("JP");
        operand("$%04X", target);
        return 3;
    }
    // ...
}
```

### Label Generation

When a reference is added:
1. Check if target address has a label
2. If not, generate automatic label:
   - `PROC_nnnn` for call targets
   - `AL_nnnn` for jump targets
   - `DATA_nnnn` for data references
3. Store label with address

### Cross-Reference Output

With `-x` flag, generates report like:
```
Cross-Reference Listing:

AL_0010  0010  2 JMP   <- 0002, 0008
PROC_0020 0020 1 CALL  <- 0005
```

## Error Handling

### Error Reporting Convention

Use descriptive error messages with context:

```c
fprintf(stderr, "%s :: Error :: Message\n", progname);
```

Examples:
- `dasmz80 :: Error :: Ran past end of input file`
- `txt2bin :: Error :: Failed to open "file.txt"`
- `dasmxx :: Warning :: No label for address $1234`

### Error Return Codes

- **0** - Success
- **1** - General error (file not found, parse error, etc.)
- **Non-zero** - Error code (EXIT_FAILURE)

### Error Handling Strategy

1. **File I/O errors:** Check return values of fopen(), fread(), fwrite()
2. **EOF handling:** Decoders return 0 from dasm_insn() on EOF
3. **Invalid input:** Print error and exit (don't try to continue)
4. **Memory allocation:** Use zalloc() which exits on failure

### Common Error Conditions

**In dasmxx.c:**
- Command file parse errors
- Input file not found
- Invalid addresses in command file
- Ran past end of file during disassembly

**In decoders:**
- Unrecognized opcodes (output "???" or "ILLEGAL")
- Truncated instructions (EOF mid-instruction)

**In txt2bin:**
- Invalid hex values
- File I/O errors

## Build System

### Makefile Organization

```makefile
# Core objects used by all disassemblers
CORE_OBJS = dasmxx.o xref.o

# Objects for table-driven disassemblers
TABLE_OBJS = $(CORE_OBJS) optab.o

# Processor-specific builds
dasmz80: $(TABLE_OBJS) decodez80.o
    $(CC) $^ -o $@

dasm96: $(CORE_OBJS) decode96.o
    $(CC) $^ -o $@
```

### Compilation Flags

Standard flags:
- `-g` - Debug symbols
- `-O2` - Optimization
- `-Wall` - All warnings

Strict flags (for testing):
- `-Wall -Wextra -Wpedantic`
- `-Wshadow -Wformat=2`
- `-Werror=implicit-function-declaration`

## Adding a New Processor

### Step 1: Create Decoder

Create `src/decode<proc>.c`:

```c
#include "dasmxx.h"
#include "optab.h"

// Define processor profile
DASM_PROFILE(
    "procname",
    max_insn_len,
    LITTLE_ENDIAN,
    ';'
);

// Define opcode tables
static optab_t main_table[] = {
    INSN("NOP", "", 0x00, 0),
    MASK("LD", "r,r", 0xC0, 0x40, 0),
    // ...
    END_TABLE
};

// Define operand functions
OPERAND_FUNC(reg_reg) {
    int dst = (opc >> 3) & 0x07;
    int src = opc & 0x07;
    operand("R%d,R%d", dst, src);
}

// Main decode function
int dasm_insn(ADDR pc, char *mnem_buf, char *operand_buf)
{
    unsigned char opc = nextbyte();
    optab_t *match = optab_match(main_table, opc);

    if (!match) {
        mnem("???");
        return 1;
    }

    mnem(match->mnem);
    if (match->operand_func) {
        match->operand_func(opc);
    }

    return optab_insn_len(match);
}
```

### Step 2: Update Makefile

Add target:
```makefile
D<PROC>_OBJS = $(TABLE_OBJS) decode<proc>.o

dasm<proc>: $(D<PROC>_OBJS)
    $(CC) $^ -o $@

# Add to TARGETS
TARGETS += dasm<proc>
```

### Step 3: Create Test Files

```bash
mkdir test/dasm<proc>
cd test/dasm<proc>

# Create test binary (using txt2bin)
cat > test.txt <<EOF
00  # NOP
01  # Some instruction
EOF

../../src/txt2bin test.txt test.bin

# Create command file
cat > test.d<proc> <<EOF
ftest.bin
c0000
e0100
EOF

# Test disassembly
../../src/dasm<proc> test.d<proc>
```

### Step 4: Create Instruction Specification

Create `tools/instruction_specs/<proc>.yaml`:

```yaml
processor: <proc>

reference:
  title: "Processor Manual"
  version: "1.0"
  url: "https://example.com/manual.pdf"

instructions:
  - category: "Data Transfer"
    variants:
      - mnemonic: "LD A,B"
        operands: "A,B"
        opcode: [0x78]
        reference: "page 42"
```

Generate comprehensive tests:
```bash
cd tools
./gen_instruction_tests.py <proc> -v
```

### Step 5: Documentation

Create `test/dasm<proc>/REFERENCE.md`:

```markdown
# <PROC> Instruction Set Reference

## Reference Documentation
- Title: ...
- Version: ...
- Publisher: ...
- URL: ...

## Instruction Set Summary
...
```

## Best Practices

### For Decoder Implementation

1. **Always report cross-references:** Call `xref_addxref()` for all address references
2. **Handle illegal opcodes:** Output "???" or "ILLEGAL" for undefined opcodes
3. **Check for EOF:** Handle truncated instructions gracefully
4. **Use helper functions:** nextbyte(), nextword(), operand(), mnem()
5. **Follow formatting conventions:** Use standard operand formats

### For Code Maintenance

1. **Avoid global state when possible:** Pass parameters explicitly
2. **Check all return values:** Especially from I/O operations
3. **Use const correctness:** Mark read-only data as const
4. **Bounds checking:** Always validate array indices and buffer sizes
5. **Error messages:** Include context (filename, address, operation)

### For Testing

1. **Test illegal opcodes:** Verify decoder handles undefined opcodes
2. **Test boundary conditions:** Address wraparound, EOF, etc.
3. **Verify cross-references:** Check that all references are reported
4. **Compare with reference:** Use datasheets to verify correctness
5. **Test all addressing modes:** Ensure complete coverage

## Common Pitfalls

1. **Not reporting cross-references:** Causes missing labels
2. **Incorrect instruction length:** Causes misalignment
3. **Buffer overflow in operand formatting:** Use snprintf, not sprintf
4. **Endianness errors:** Use nextword() instead of manual byte fetching
5. **Memory leaks:** Free labels and xref entries
6. **Format string vulnerabilities:** Never use user input as format string

## Version History

- **2026-03-24**: Initial architecture documentation
- Covers current codebase structure
- Documents decoder interface and conventions
- Provides guidelines for new processor support

## See Also

- `CLAUDE.md` - Project guidelines for Claude Code
- `IMPLEMENTATION_STATUS.md` - Current implementation status
- `BUGS.md` - Known issues and bug tracking
- `test/framework/README.md` - Testing framework documentation
