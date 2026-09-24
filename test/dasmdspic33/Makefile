DIFF ?= diff -u
XC16_AS ?= xc16-as
XC16_OBJCOPY ?= xc16-objcopy

test: test.out cfg.out
	$(DIFF) test.expected test.out
	$(DIFF) cfg.expected cfg.out

test.out: test.hex test.ddspic33 ../../src/dasmdspic33
	../../src/dasmdspic33 -s test.ddspic33 | sed -n '/^___CL_0001:/,$$p' | sed '$${/^$$/d;}' > test.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.ddspic33 ../../src/dasmdspic33
	../../src/dasmdspic33 -C cfg.ddspic33 > cfg.out

regen: test.o
	$(XC16_OBJCOPY) -O ihex test.o test.hex

test.o: test.s
	$(XC16_AS) -p33FJ64GP802 -o test.o test.s

clean:
	rm -f test.err test.lst test.o test.out cfg.bin cfg.out
