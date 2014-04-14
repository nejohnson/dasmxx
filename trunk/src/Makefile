#################################################
#
# Makefile for
#
#      dasmxx
#
#  disassembler suite
#
#################################################

TARGETS = dasm78k3 dasm96 dasm02

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
	
clean:
	rm -f ${TARGETS} *.o

#################################################
