
test: test.bin test.d68k test68010.bin test68010.d68k
	../../src/dasm68k test.d68k
	../../src/dasm68k test68010.d68k

test.bin: test.txt
	../../src/txt2bin test.txt test.bin

test68010.bin: test68010.txt
	../../src/txt2bin test68010.txt test68010.bin

clean:
	rm -f test.bin test68010.bin
