/*****************************************************************************
 *
 * Copyright (C) 2014, Neil Johnson
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

/*****************************************************************************/
/*                              System / Utility                             */
/*****************************************************************************/

extern void error( char *fmt, ... );
extern void *zalloc( size_t n );
extern int next( FILE* fp, int *addr );
extern char * dupstr( const char *s );

/*****************************************************************************/
/*                              Cross Referencing                            */
/*****************************************************************************/

#define X_JMP           0
#define X_CALL          1
#define X_IMM           2
#define X_TABLE         3
#define X_DIRECT        4
#define X_DATA          5
#define X_PTR           6

extern void xref_setmin( int min );
extern void xref_setmax( int max );
extern int  xref_inrange( int ref );
extern void xref_addxref( int type, int addr, int ref );
extern void xref_addxreflabel( int ref, char *label );
extern char * xref_findaddrlabel( int addr );
extern char * xref_genwordaddr( char * buf, const char * prefix, int addr );
extern void xref_dump( void );

/*****************************************************************************/
/*                              Disassembler                                 */
/*****************************************************************************/

extern int dasm_insn( FILE *f, char * outbuf, int addr );
extern char * dasm_name;
extern char * dasm_description;

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
