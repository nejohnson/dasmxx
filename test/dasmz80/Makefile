
DIFF ?= diff -u

test: testz180.out testz80strict.out cfg.out
	../../src/txt2bin test.txt test.bin
	../../src/dasmz80 test.dz80
	$(DIFF) testz180.expected testz180.out
	$(DIFF) testz80strict.expected testz80strict.out
	$(DIFF) cfg.expected cfg.out

testz180.bin: testz180.txt ../../src/txt2bin
	../../src/txt2bin testz180.txt testz180.bin

testz180.out: testz180.bin testz180.dz80 ../../src/dasmz80
	../../src/dasmz80 -s testz180.dz80 | sed '$${/^$$/d;}' > testz180.out

testz80strict.bin: testz80strict.txt ../../src/txt2bin
	../../src/txt2bin testz80strict.txt testz80strict.bin

testz80strict.out: testz80strict.bin testz80strict.dz80 ../../src/dasmz80
	../../src/dasmz80 -s testz80strict.dz80 | sed '$${/^$$/d;}' > testz80strict.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.dz80 ../../src/dasmz80
	../../src/dasmz80 -C cfg.dz80 > cfg.out

clean:
	rm -f test.bin testz180.bin testz180.out testz80strict.bin testz80strict.out cfg.bin cfg.out
