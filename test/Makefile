# Makefile for the dasmxx test suite.

TEST_DIRS = \
	dasm02 \
	dasm05 \
	dasm09 \
	dasm1802 \
	dasm430 \
	dasm48 \
	dasm51 \
	dasm6800 \
	dasm6811 \
	dasm68k \
	dasm7000 \
	dasm78k3 \
	dasm85 \
	dasm96 \
	dasm9900 \
	dasmavr \
	dasmdspic33 \
	dasmm8 \
	dasmpic12 \
	dasmpic16 \
	dasmpic18 \
	dasmpic24 \
	dasmunsp \
	dasmx86 \
	dasmz8 \
	dasmz80 \
	tool_features

.PHONY: test clean $(TEST_DIRS)

test:
	$(MAKE) -C ../src all
	@for d in $(TEST_DIRS); do \
		echo "==> $$d"; \
		$(MAKE) -C $$d test || exit $$?; \
	done

clean:
	@for d in $(TEST_DIRS); do \
		$(MAKE) -C $$d clean || exit $$?; \
	done
	$(MAKE) -C ../src clean
