
test: 
	../../src/txt2bin test.txt test.bin
	mkdir -p output
	../../src/dasm05 test.d05 > output/test.out
	grep -F 'bset     3, $$88' output/test.out
	grep -F 'ror      $$88' output/test.out
	grep -F 'jmp      TestAddr1' output/test.out
	grep -F 'jsr      TestAddr1' output/test.out
	! grep -F 'bset     3, TestAddr1' output/test.out
	! grep -F 'ror      TestAddr1' output/test.out

clean:
	rm -f test.bin
	rm -rf output/
