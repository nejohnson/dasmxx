
DIFF ?= diff -u

test: cfg.out
	../../src/txt2bin test.txt test.bin
	../../src/dasmx86 test.dx86
	../../src/txt2bin test8086.txt test8086.bin
	../../src/dasmx86 test8086.dx86
	../../src/txt2bin test286.txt test286.bin
	../../src/dasmx86 test286.dx86
	../../src/txt2bin test386.txt test386.bin
	../../src/dasmx86 test386.dx86
	$(DIFF) cfg.expected cfg.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.dx86 ../../src/dasmx86
	../../src/dasmx86 -C cfg.dx86 > cfg.out

clean:
	rm -f test.bin test8086.bin test286.bin test386.bin cfg.bin cfg.out
