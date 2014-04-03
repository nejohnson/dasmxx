#################################################
#
# Makefile for
#
#      dasmxx
#
#  disassembler suite
#
#################################################

TARGETS = dasm78k3 dasm96

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
	
clean:
	rm -f ${TARGETS} *.o

#################################################
