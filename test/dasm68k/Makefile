
test: test.bin test.d68k test68010.bin test68010.d68k test68020.bin test68020.d68k test68030.bin test68030.d68k
	../../src/dasm68k test.d68k
	../../src/dasm68k test68010.d68k
	../../src/dasm68k test68020.d68k
	../../src/dasm68k test68030.d68k

test.bin: test.txt
	../../src/txt2bin test.txt test.bin

test68010.bin: test68010.txt
	../../src/txt2bin test68010.txt test68010.bin

test68020.bin: test68020.txt
	../../src/txt2bin test68020.txt test68020.bin

test68030.bin: test68030.txt
	../../src/txt2bin test68030.txt test68030.bin

clean:
	rm -f test.bin test68010.bin test68020.bin test68030.bin
