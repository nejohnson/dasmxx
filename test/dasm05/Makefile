
DIFF ?= diff -u

test: cfg.out
	../../src/txt2bin test.txt test.bin
	mkdir -p output
	../../src/dasm05 test.d05 > output/test.out
	grep -F 'bset     3, $$88' output/test.out
	grep -F 'ror      $$88' output/test.out
	grep -F 'jmp      TestAddr1' output/test.out
	grep -F 'jsr      TestAddr1' output/test.out
	! grep -F 'bset     3, TestAddr1' output/test.out
	! grep -F 'ror      TestAddr1' output/test.out
	$(DIFF) cfg.expected cfg.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d05 ../../src/dasm05
	../../src/dasm05 -C cfg.d05 > cfg.out

clean:
	rm -f test.bin cfg.bin cfg.out
	rm -rf output/
