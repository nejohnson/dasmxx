DIFF ?= diff -u

test: test.out testx.out cfg.out cfgx.out
	$(DIFF) test.expected test.out
	$(DIFF) testx.expected testx.out
	$(DIFF) cfg.expected cfg.out
	$(DIFF) cfgx.expected cfgx.out

test.bin: test.txt ../../src/txt2bin
	../../src/txt2bin test.txt test.bin

testx.bin: testx.txt ../../src/txt2bin
	../../src/txt2bin testx.txt testx.bin

test.out: test.bin test.d430 ../../src/dasm430
	../../src/dasm430 -s test.d430 | sed '$${/^$$/d;}' > test.out

testx.out: testx.bin testx.d430 ../../src/dasm430
	../../src/dasm430 -s testx.d430 | sed '$${/^$$/d;}' > testx.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfgx.bin: cfgx.txt ../../src/txt2bin
	../../src/txt2bin cfgx.txt cfgx.bin

cfg.out: cfg.bin cfg.d430 ../../src/dasm430
	../../src/dasm430 -C cfg.d430 > cfg.out

cfgx.out: cfgx.bin cfgx.d430 ../../src/dasm430
	../../src/dasm430 -C cfgx.d430 > cfgx.out

clean:
	rm -f test.out test.bin testx.out testx.bin cfg.out cfg.bin cfgx.out cfgx.bin
