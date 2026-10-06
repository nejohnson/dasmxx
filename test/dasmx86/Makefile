
DIFF ?= diff -u
SHELL := /bin/bash
CASES = test test8086 test286 test386

test: $(addprefix output/,$(addsuffix .out,$(CASES))) cfg.out
	$(DIFF) test.expected output/test.out
	$(DIFF) test8086.expected output/test8086.out
	$(DIFF) test286.expected output/test286.out
	$(DIFF) test386.expected output/test386.out
	$(DIFF) cfg.expected cfg.out

%.bin: %.txt ../../src/txt2bin
	../../src/txt2bin $< $@

output/%.out: %.bin %.dx86 ../../src/dasmx86
	mkdir -p output
	set -o pipefail; ../../src/dasmx86 -s $*.dx86 | sed 's/[[:space:]]*$$//' | sed '$${/^$$/d;}' > $@

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.dx86 ../../src/dasmx86
	../../src/dasmx86 -C cfg.dx86 > cfg.out

clean:
	rm -f $(addsuffix .bin,$(CASES)) cfg.bin cfg.out
	rm -rf output/

.PHONY: test clean
.SECONDARY: $(addsuffix .bin,$(CASES))
