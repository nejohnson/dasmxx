
DIFF ?= diff -u

test: test.out cfg.out
	$(DIFF) test.expected test.out
	$(DIFF) cfg.expected cfg.out

test.out: test.hex test.d96 ../../src/dasm96
	../../src/dasm96 -s test.d96 | sed '$${/^$$/d;}' | sed 's/[[:space:]]*$$//' > test.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d96 ../../src/dasm96
	../../src/dasm96 -C cfg.d96 > cfg.out

clean:
	rm -f test.out cfg.bin cfg.out
