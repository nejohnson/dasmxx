
DIFF ?= diff -u
CASES = test test68010 test68020 test68030 test68040 test68881 test68040fpu

test: $(addprefix output/,$(addsuffix .out,$(CASES))) cfg.out
	$(DIFF) test.expected output/test.out
	$(DIFF) test68010.expected output/test68010.out
	$(DIFF) test68020.expected output/test68020.out
	$(DIFF) test68030.expected output/test68030.out
	$(DIFF) test68040.expected output/test68040.out
	$(DIFF) test68881.expected output/test68881.out
	$(DIFF) test68040fpu.expected output/test68040fpu.out
	$(DIFF) cfg.expected cfg.out

%.bin: %.txt ../../src/txt2bin
	../../src/txt2bin $< $@

output/%.out: %.bin %.d68k ../../src/dasm68k
	mkdir -p output
	../../src/dasm68k -s $*.d68k | sed 's/[[:space:]]*$$//' | sed '$${/^$$/d;}' > $@

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d68k ../../src/dasm68k
	../../src/dasm68k -C cfg.d68k > cfg.out

clean:
	rm -f $(addsuffix .bin,$(CASES)) cfg.bin cfg.out
	rm -rf output/

.PHONY: test clean
.SECONDARY: $(addsuffix .bin,$(CASES))
