#################################################
#
# Makefile for
#
#      dasmxx
#
#  disassembler suite
#
#################################################

TARGETS = dasm78k dasm96

CORE_OBJS = dasmxx.o xref.o

CFLAGS = -g

all:	${TARGETS}

#################################################

D78K_OBJS = ${CORE_OBJS} decode78k.o

dasm78k: ${D78K_OBJS}
	$(CC) ${D78K_OBJS} -o dasm78k

#################################################

D96_OBJS = ${CORE_OBJS} decode96.o

dasm96: ${D96_OBJS}
	$(CC) ${D96_OBJS} -o dasm96

#################################################
	
clean:
	rm -f ${TARGETS} *.o

#################################################
