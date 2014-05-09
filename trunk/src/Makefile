#################################################
#
# Makefile for
#
#      dasmxx
#
#  disassembler suite
#
#################################################

TARGETS = dasm78k3 dasm96 dasm02 dasm7000 dasm09

CORE_OBJS = dasmxx.o xref.o

CFLAGS = -g

all:	${TARGETS}

#################################################

D78K3_OBJS = ${CORE_OBJS} decode78k3.o

dasm78k3: ${D78K3_OBJS}
	$(CC) ${D78K3_OBJS} -o ${@}

#################################################

D96_OBJS = ${CORE_OBJS} decode96.o

dasm96: ${D96_OBJS}
	$(CC) ${D96_OBJS} -o ${@}

#################################################

D02_OBJS = ${CORE_OBJS} decode02.o

dasm02: ${D02_OBJS}
	$(CC) ${D02_OBJS} -o ${@}

#################################################

D09_OBJS = ${CORE_OBJS} decode09.o

dasm09: ${D09_OBJS}
	$(CC) ${D09_OBJS} -o ${@}

#################################################

D7000_OBJS = ${CORE_OBJS} decode7000.o

dasm7000: ${D7000_OBJS}
	$(CC) ${D7000_OBJS} -o ${@}
	
#################################################
	
clean:
	rm -f ${TARGETS} *.o

#################################################
