
DIFF ?= diff -u

test: output/test.out output/test_page.out cfg.out
	$(DIFF) test.expected output/test.out
	$(DIFF) test_page.expected output/test_page.out
	$(DIFF) cfg.expected cfg.out

test.bin: test.txt ../../src/txt2bin
	../../src/txt2bin test.txt test.bin

test_page.bin: test_page.txt ../../src/txt2bin
	../../src/txt2bin test_page.txt test_page.bin

output/test.out: test.bin test.d48 ../../src/dasm48
	mkdir -p output
	../../src/dasm48 -s test.d48 | sed 's/[[:space:]]*$$//' | sed '$${/^$$/d;}' > output/test.out

output/test_page.out: test_page.bin test_page.d48 ../../src/dasm48
	mkdir -p output
	../../src/dasm48 -s test_page.d48 | sed 's/[[:space:]]*$$//' | sed '$${/^$$/d;}' > output/test_page.out

cfg.bin: cfg.txt ../../src/txt2bin
	../../src/txt2bin cfg.txt cfg.bin

cfg.out: cfg.bin cfg.d48 ../../src/dasm48
	../../src/dasm48 -C cfg.d48 > cfg.out

clean:
	rm -f test.bin test_page.bin cfg.bin cfg.out
	rm -rf output/

.PHONY: test clean
