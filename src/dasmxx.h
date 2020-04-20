/*****************************************************************************
 *
 * Copyright (C) 2014-2015, Neil Johnson
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms,
 * with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of Neil Johnson nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
 
#ifndef _DASMXX_H_
#define _DASMXX_H_

/*****************************************************************************/
/*                              Common Macros                                */
/*****************************************************************************/

#define MIN(a,b)        ((a)<(b)?(a):(b))
#define MAX(a,b)        ((a)>(b)?(a):(b))

/*****************************************************************************/
/*                              Machine Types                                */
/*****************************************************************************/

/* Base types */
typedef unsigned char      UBYTE;
typedef signed char        BYTE;
typedef unsigned short     UWORD;
typedef signed short       WORD;
typedef unsigned int       ADDR;
typedef signed int         LWORD;
typedef unsigned int       ULWORD;

/* Derived types */
typedef UWORD              OPC;

/* Universal address format */
#define FORMAT_ADDR		"%04X"

/* Prefix for generated labels */
#define GEN_LABEL_PREFIX    "___"

/*****************************************************************************/
/*                              System / Utility                             */
/*****************************************************************************/

extern void error( char *fmt, ... );
extern void warning( char *fmt, ... );
extern void *zalloc( size_t n );
extern UBYTE next( FILE* fp, ADDR *addr );
extern UWORD nextw( FILE *fp, ADDR *addr );
extern UBYTE peek( FILE *fp );
extern char * dupstr( const char *s );

/*****************************************************************************/
/*                              Cross Referencing                            */
/*****************************************************************************/

typedef enum {
   X_NONE   = -1,
   X_JMP    = 0,
   X_CALL   = 1,
   X_IMM    = 2,
   X_TABLE  = 3,
   X_DIRECT = 4,
   X_DATA   = 5,
   X_PTR    = 6,
   X_REG    = 7,
   X_IO     = 8
} XREF_TYPE;

extern void xref_addxref( XREF_TYPE type, ADDR addr, ADDR ref );
extern void xref_addxreflabel( ADDR ref, char *label );
extern char * xref_findaddrlabel( ADDR addr );
extern char * xref_genwordaddr( char * buf, const char * format, ADDR addr );
extern void xref_dump( void );

/*****************************************************************************/
/*                              Disassembler                                 */
/*****************************************************************************/

extern ADDR dasm_insn( FILE *f, char * outbuf, ADDR addr );
extern const char * dasm_name;
extern const char * dasm_description;
extern const int    dasm_max_insn_length;
extern const int    dasm_max_opcode_width;
extern const int    dasm_word_msb_first;
extern const int    dasm_insn_width_bytes;

#define DASM_PROFILE(name,desc,insnlen,opwid,msb,iwid) \
    const char * dasm_name = name;                /* Name of assembler     */ \
    const char * dasm_description = desc;         /* Target description    */ \
    const int    dasm_max_insn_length = insnlen;  /* Max bytes per insn    */ \
    const int    dasm_max_opcode_width = opwid;   /* Max chars insn name   */ \
    const int    dasm_word_msb_first = msb;       /* 1 if word is MSB first*/ \
    const int    dasm_insn_width_bytes = iwid;    /* Num bytes per opcode  */

/*****************************************************************************/

#endif

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
