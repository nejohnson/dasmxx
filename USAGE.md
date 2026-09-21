Usage
=====

This program makes use of a command file, specified on the command line at
runtime.  This file contains the name of the input file, together with
various disassembling commands to aid the disassembler make sense of the
memory map, and the different types of data (or code) within different
segments of memory.

Command line
============

     dasmXX [options] listfile

Where

- XX         - target name (78k, 96, etc)
- listfile   - is the name of the command list file

Supported command line options are:

     -h         - print helpful usage information
     -x         - generate cross-reference list at end of disassembly
     -a         - generate assembler source output
     -s         - generate stripped assembler output (forces -a)
     -o foo     - write output to file "foo" (default is stdout)

Command list file
=================

The command list file contains a list of memory segment definitions, used during
 processing to tell the disassembler what the memory at a particular address
 is for (unknown, code, or some sort of data).

The commands are (where XXXX denotes hexadecimal address):

File commands:

     fName       input file = `Name' (raw binary, Intel HEX, or Motorola S-record)
     f@XXXX Name input file = `Name', loaded at address XXXX
     iName       include file `Name' in place of include command
     >XXXX       fast forward to offset XXXX from start of binary input files

For raw binary input, `fName' loads the file at the first disassembly address.
Use `f@XXXX Name' to place a binary ROM image at a specific address.  Multiple
`f@' commands may be used for split ROM sets.  Intel HEX and Motorola S-record
input normally use the addresses encoded in the file; with `f@XXXX Name', those
record addresses are relocated by XXXX.  The `>' offset applies only to binary
input, and applies to every binary input file in the command list.

Configuration commands:

     tXX         string terminator byte (default = 00)
     gXXXX       segment base for following code entries
     oName=Value decoder option (for example, ocpu=8086 or ofpu=68881)
     eXXXX       end of disassembly
     q[,N]["title"]  pagination, N lines (default=60), optional title

Supported `ocpu` values include `6502`, `65c02`, `65816`, `65c816`,
`z80`, `z180`, `hd64180`, `8086`, `80186`, `80286`, `80386`, `80486`,
`6800`, `6801`, `6803`, `6811`, `68hc11`, `68000`, `68010`, `68020`,
`68030`, `68040`, `msp430`, `msp430x`, and `cpux`.
For 65816 immediate operands, use `oacc=8` or `oacc=16` for accumulator
width and `oidx=8` or `oidx=16` for index-register width.

Dump commands:

     aXXXX       alphanumeric dump
     bXXXX[,N]   byte dump (N bytes, default is 16)
     mXXXX       bitmap (each byte is dumped as a string of # and . for 1 and 0 bits respectively)
     sXXXX       string dump
     uXXXX       string dump with 16-bit characters (utf-16)
     vXXXX       vector address dump
     wXXXX       word dump
     zXXXX       skip (emits a SKIP with the number of bytes). Source must already be 0-filled.

Code disassembly commands:

     cXXXX       code disassembly starts at XXXX
     pXXXX       procedure start
     lXXXX       attach a label to address XXXX
     kXXXX       one-line (k)comment for address XXXX
     nXXXX       multi-line block comment, ends with line starting '.'

Commands c,b,s,e,w,a,p,l can have a comment string separated from the
address by whitespace (tab or space).  The comment is printed in
the listing.

The labels attached via 'p' and 'l' will be used both in the XREF dump
at the end, and within the disassembly.  For example:

      p1234    TestFunc

identifies address 1234 as the entry point of procedure "TestFunc", and
this label will be used in the disassembly, as in:

      ljmp      TestFunc

rather than the less-readable:

      ljmp      1234

The 'l' command is similar, and can be used to identify branch or jump
targets (loops, tests, etc) and data (tables, strings, etc).

Note: both 'l' and 'p' use auto-naming: if no name is given then dasmxx
will generate a name for you: "AL_nnnn" for labels, and "PROC_nnnn" for 
procedures.
