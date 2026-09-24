
DIFF ?= diff -u

test: test.out cfg.out
	$(DIFF) test.expected test.out
	$(DIFF) cfg.expected cfg.out

test.out: test.hex test.dunsp ../../src/dasmunsp
	../../src/dasmunsp -s test.dunsp | sed '$${/^$$/d;}' | sed 's/[[:space:]]*$$//' > test.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.dunsp ../../src/dasmunsp
	../../src/dasmunsp -C cfg.dunsp > cfg.out

clean:
	rm -f test.out cfg.bin cfg.out
