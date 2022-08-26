#################################################
#
# Makefile for
#
#      dasmxx
#
#  disassembler suite
#
#################################################

# If building on Dos/Windows we need the .exe extension
ifdef COMSPEC
X=.exe
endif

TARGETS = dasm78k3$(X) \
          dasm96$(X)   \
          dasm02$(X)   \
          dasm05$(X)   \
          dasm7000$(X) \
          dasm09$(X)   \
          dasmavr$(X)  \
          dasm51$(X)   \
          dasmz80$(X)  \
          dasm48$(X)   \
          dasmx86$(X)  \
          dasm85$(X)   \
          dasm1802$(X) \
          dasm68k$(X)  \
          dasmunsp$(X)  \
          txt2bin$(X)

CORE_OBJS = dasmxx.o xref.o optab.o

# Special-case the 8096 until it is re-written.
CORE96_OBJS = dasmxx.o xref.o

CFLAGS = -g

all:	$(subst $(X),,${TARGETS})

#################################################

D78K3_OBJS = ${CORE_OBJS} decode78k3.o

dasm78k3: ${D78K3_OBJS}
	$(CC) ${D78K3_OBJS} -o ${@}

#################################################

D96_OBJS = ${CORE96_OBJS} decode96.o

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

DAVR_OBJS = ${CORE_OBJS} decodeavr.o

dasmavr: ${DAVR_OBJS}
	$(CC) ${DAVR_OBJS} -o ${@}

#################################################

D51_OBJS = ${CORE_OBJS} decode51.o

dasm51: ${D51_OBJS}
	$(CC) ${D51_OBJS} -o ${@}
	
#################################################

DZ80_OBJS = ${CORE_OBJS} decodez80.o

dasmz80: ${DZ80_OBJS}
	$(CC) ${DZ80_OBJS} -o ${@}

#################################################

D48_OBJS = ${CORE_OBJS} decode48.o

dasm48: ${D48_OBJS}
	$(CC) ${D48_OBJS} -o ${@}

#################################################

D05_OBJS = ${CORE_OBJS} decode05.o

dasm05: ${D05_OBJS}
	$(CC) ${D05_OBJS} -o ${@}

#################################################

DX86_OBJS = ${CORE_OBJS} decodex86.o

dasmx86: ${DX86_OBJS}
	$(CC) ${DX86_OBJS} -o ${@}

#################################################

D85_OBJS = ${CORE_OBJS} decode85.o

dasm85: ${D85_OBJS}
	$(CC) ${D85_OBJS} -o ${@}

#################################################

D1802_OBJS = ${CORE_OBJS} decode1802.o

dasm1802: ${D1802_OBJS}
	$(CC) ${D1802_OBJS} -o ${@}

#################################################

D68K_OBJS = ${CORE_OBJS} decode68k.o

dasm68k: ${D68K_OBJS}
	$(CC) ${D68K_OBJS} -o ${@}

#################################################

DUNSP_OBJS = ${CORE_OBJS} decodeunsp.o

dasmunsp: ${DUNSP_OBJS}
	$(CC) ${DUNSP_OBJS} -o ${@}

#################################################

txt2bin: txt2bin.c
	$(CC) $< -o $@

#################################################
	
clean:
	rm -f ${TARGETS} *.o

#################################################
