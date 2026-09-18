XC16_BINDIR ?= /opt/microchip/xc16/v2.10/bin
XC16_AS = $(XC16_BINDIR)/xc16-as
XC16_OBJCOPY = $(XC16_BINDIR)/xc16-objcopy
DIFF ?= diff -u

test: test.out
	$(DIFF) test.expected test.out

test.out: test.hex test.dpic24 ../../src/dasmpic24
	../../src/dasmpic24 -s test.dpic24 | sed -n '/^___CL_0001:/,$$p' | sed '$${/^$$/d;}' > test.out

test.hex: test.o
	$(XC16_OBJCOPY) -O ihex test.o test.hex

test.o: test.s
	$(XC16_AS) -p24FJ64GA002 -o test.o test.s

clean:
	rm -f test.err test.hex test.lst test.o test.out
