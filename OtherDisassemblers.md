# Introduction #

dasmxx is by no means the first multi-device scriptable disassembler.  This page documents a number of them, with comments where appropriate.

# Ghidra #

[Ghidra](https://ghidra-sre.org/) is a "software reverse engineering (SRE) suite of tools developed by NSA's Research Directorate in support of the Cybersecurity mission". It is a Java-based tool that looks extremely powerful.

# IDA #

[IDA](https://www.hex-rays.com/products/ida/) is probably the most widely known multi-device disassembler.  It started life as a shareware project, and as its popularity grew it became a commercial product.  Older versions are still available on the internet, obviously without the latest features and limited device support.  However it is possible to write your own disassembler plugins, and there is support for the NEC 78K0 family, so if I wanted to spend money I would probably choose this product and extend it.

# METASM #

[METASM](http://metasm.cr0.org/) is _"a cross-architecture assembler, disassembler, compiler, linker and debugger"_.  It looks very capable, with lots of tools and visualisers (graphical output).  However, it is aimed at larger, supporting only IA32, MIPS and PPC.  It has been suggested that it has been integrated into [Metasploit](http://www.metasploit.com/).  While it is written in Ruby, and open-source, it would probably be overkill to extend it to support the NEC 78K3 series (the starting point of dasmxx).

