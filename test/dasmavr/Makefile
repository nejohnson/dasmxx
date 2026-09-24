DIFF ?= diff -u
AVR_AS ?= avr-as
AVR_LD ?= avr-ld
AVR_OBJCOPY ?= avr-objcopy

test: test.out cfg.out
	$(DIFF) test.expected test.out
	$(DIFF) cfg.expected cfg.out

test.out: test.hex test.davr ../../src/dasmavr
	../../src/dasmavr -s test.davr | sed '$${/^$$/d;}' | sed 's/[[:space:]]*$$//' > test.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.davr ../../src/dasmavr
	../../src/dasmavr -C cfg.davr > cfg.out

regen: test.elf
	$(AVR_OBJCOPY) -O ihex test.elf test.hex
	tr -d '\r' < test.hex > test.hex.tmp
	mv test.hex.tmp test.hex

test.elf: test.o
	$(AVR_LD) -m avr5 -Ttext=0 -o test.elf test.o

test.o: test.s
	$(AVR_AS) -mmcu=atmega128 -o test.o test.s

clean:
	rm -f test.elf test.hex.tmp test.o test.out cfg.bin cfg.out
