# Introduction #

dasmxx is by no means the first multi-device scriptable disassembler.  This page documents a number of them, with comments where appropriate.

# Ghidra #

[Ghidra](https://ghidra-sre.org/) is a "software reverse engineering (SRE) suite of tools developed by NSA's Research Directorate in support of the Cybersecurity mission". It is a Java-based tool that looks extremely powerful.

# DASMx #

[DASMx](http://myweb.tiscali.co.uk/pclare/DASMx/) is _"a disassembler for a range of common microprocessors"_.  It was only available for the Windows platform.  It is a closed-source application, and was last updated in 2003.  It supports a wide range of devices, from the 6800/6809/6502/Z80/8051/PIC families, to some esoteric devices like the Signetics 2650 and CDP1802.

One nice feature it does have is a form of automated disassembly: once given one or more start points (e.g., reset and interrupt vectors) it walks the instructions partially interpreting call/jump/branch instructions to work out branch labels and procedure entry points.

Two strikes against it: it doesn't support the NEC 78K family, and being closed source it is impossible to add support to it.

# IDA #

[IDA](https://www.hex-rays.com/products/ida/) is probably the most widely known multi-device disassembler.  It started life as a shareware project, and as its popularity grew it became a commercial product.  Older versions are still available on the internet, obviously without the latest features and limited device support.  However it is possible to write your own disassembler plugins, and there is support for the NEC 78K0 family, so if I wanted to spend money I would probably choose this product and extend it.

# METASM #

[METASM](http://metasm.cr0.org/) is _"a cross-architecture assembler, disassembler, compiler, linker and debugger"_.  It looks very capable, with lots of tools and visualisers (graphical output).  However, it is aimed at larger, supporting only IA32, MIPS and PPC.  It has been suggested that it has been integrated into [Metasploit](http://www.metasploit.com/).  While it is written in Ruby, and open-source, it would probably be overkill to extend it to support the NEC 78K3 series (the starting point of dasmxx).

# ODAWeb #

[ODAWeb](http://www.onlinedisassembler.com/odaweb/) is an online disassembler.  It supports a wide range of devices -- very similar to what GCC supports, so I'm guessing it is based on the GCC source tree.

