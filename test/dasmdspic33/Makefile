DIFF ?= diff -u
XC16_AS ?= xc16-as
XC16_OBJCOPY ?= xc16-objcopy

test: test.out
	$(DIFF) test.expected test.out

test.out: test.hex test.ddspic33 ../../src/dasmdspic33
	../../src/dasmdspic33 -s test.ddspic33 | sed -n '/^___CL_0001:/,$$p' | sed '$${/^$$/d;}' > test.out

regen: test.o
	$(XC16_OBJCOPY) -O ihex test.o test.hex

test.o: test.s
	$(XC16_AS) -p33FJ64GP802 -o test.o test.s

clean:
	rm -f test.err test.lst test.o test.out
