XC16_BINDIR ?= /opt/microchip/xc16/v2.10/bin
XC16_AS = $(XC16_BINDIR)/xc16-as
XC16_OBJCOPY = $(XC16_BINDIR)/xc16-objcopy
DIFF ?= diff -u

test: test.out
	$(DIFF) test.expected test.out

test.out: test.hex test.ddspic33 ../../src/dasmdspic33
	../../src/dasmdspic33 -s test.ddspic33 | sed -n '/^___CL_0001:/,$$p' | sed '$${/^$$/d;}' > test.out

test.hex: test.o
	$(XC16_OBJCOPY) -O ihex test.o test.hex

test.o: test.s
	$(XC16_AS) -p33FJ64GP802 -o test.o test.s

clean:
	rm -f test.err test.hex test.lst test.o test.out
