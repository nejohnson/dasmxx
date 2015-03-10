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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "dasmxx.h"



const char * dasm_name            = "dasm96";
const char * dasm_description     = "Intel 8096";
const int    dasm_max_insn_length = 8;
const int    dasm_max_opcode_width = 9;


static char * output_buffer = NULL;


#define ADDR_DIRECT     0
#define ADDR_IMMED      1
#define ADDR_INDIR      2
#define ADDR_INDEX      3


#define FORMAT_NUM_16BIT        "%04X"


/*****************************************************************************
 *        Instruction Decoding Tables
 *****************************************************************************/

char instrlen[] = {
/* 0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f  */
   2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,   /* 0 */
   0, 2, 2, 2, 0, 2, 2, 2, 3, 3, 3, 0, 0, 0, 0, 0,   /* 1 */
   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,   /* 2 */
   3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,   /* 3 */
   4, 5, 4,-5, 4, 5, 4,-5, 4, 5, 4,-5, 4, 5, 4,-5,   /* 4 */
   4, 4, 4,-5, 4, 4, 4,-5, 4, 4, 4,-5, 4, 4, 4,-5,   /* 5 */
   3, 4, 3,-4, 3, 4, 3,-4, 3, 4, 3,-4, 3, 4, 3,-4,   /* 6 */
   3, 3, 3,-4, 3, 3, 3,-4, 3, 3, 3,-4, 3, 3, 3,-4,   /* 7 */
   3, 4, 3,-4, 3, 4, 3,-4, 3, 4, 3,-4, 3, 4, 3,-4,   /* 8 */
   3, 3, 3,-4, 3, 3, 3,-4, 3, 3, 3,-4, 3, 3, 3,-4,   /* 9 */
   3, 4, 3,-4, 3, 4, 3,-4, 3, 4, 3,-4, 3, 3, 3,-4,   /* a */
   3, 3, 3,-4, 3, 3, 3,-4, 3, 3, 3,-4, 3, 3, 3,-4,   /* b */
   3, 3, 3,-4, 3, 3, 3,-4, 2, 3, 2,-3, 2, 0, 2,-3,   /* c */
   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,   /* d */
   3, 3, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 3,   /* e */
   1, 0, 1, 1, 1, 1, 2, 0, 1, 1, 1, 1, 1, 1,-7, 1    /* f */
};
/*

00100bbb xxxxxxxx    -> sjmp  bbbxxxxxxxx
00101bbb xxxxxxxx    -> scall bbbxxxxxxxx
00110bbb src ofs     -> jbc src, bb, ofs
00111bbb src ofs     -> jbs src, bb, ofs
010....0   b0 b1 b2
010...01   b0 b1 b2 b3
010...11   b0 b1 b2 b3 b4  (b4 als b0&1)
0110...0   b0 b1
0110..01   b0 b1 b2
0110..11   b0 b1 b2 b3  (b3 als b0&1)
0111...0   b0 b1
0111..01   b0 b1
0111..11   b0 b1 b2 b3  (b3 als b0&1)
10.0...0   b0 b1
10.0..01   b0 b1 b2
10.0..11   b0 b1 b2 b3  (b3 als b0&1)
10.1...0   b0 b1
10.1..01   b0 b1
10.1..11   b0 b1 b2 b3  (b3 als b0&1)
11001..0   b0
11000..0   b0 b1
11001001   b0 b1
11001.11   b0 b1 b2   (b2 als b0&1)
11000.11   b0 b1 b2 b3  (b3 als b0&1)
1101....   ofs      j.. ofs
*/

/*
   0 = unknown opcode
   >0 = # bytes
   <0 : +1 if 2nd byte odd
   -7 : extended opcode
*/


/*****************************************************************************
 *        Private Functions
 *****************************************************************************/
 
static void opcode( const char *opcode )
{
	int n = sprintf( output_buffer, "%-*s", dasm_max_opcode_width, opcode );
	output_buffer += n;
}

static void operand( const char *operand, ... )
{
	va_list ap;
	int n;
	
	va_start( ap, operand );
	n = vsprintf( output_buffer, operand, ap );
	va_end( ap );
	
	output_buffer += n;
}

/***********************************************************
 *
 ***********************************************************/

union {
	struct {
#ifdef __BIG_ENDIAN__	
		unsigned char hibyte;
		unsigned char lobyte;
#else
		unsigned char lobyte;
		unsigned char hibyte;
#endif		
	} s;
	unsigned short us_val;
	signed   short s_val;
} u;

static unsigned short getAddress( unsigned char * buf )
{
	u.s.lobyte = buf[0];
	u.s.hibyte = buf[1];
		
	return u.us_val;
}

static short getOffset( unsigned char * buf )
{
	u.s.lobyte = buf[0];
	u.s.hibyte = buf[1];
	
	return u.s_val;
}

/***********************************************************
 *
 * FUNCTION
 *      do_sjmp
 *
 * DESCRIPTION
 *      Handle sjmp (short jump) instructions
 *
 * RETURNS
 *      void
 *
 ************************************************************/

static void do_sjmp( int addr, unsigned char *buf, int n )
{
    short offset;
    
    offset = ( ( buf[0] & 0x03 ) << 8 ) | buf[1];
    
    if ( buf[0] & 4 )
        offset |= 0xFC00;
    
    operand( "sjmp    %s", xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr + offset ) );
    xref_addxref( X_JMP, addr - n, addr + offset );
}

/***********************************************************
 *
 * FUNCTION
 *      do_scall
 *
 * DESCRIPTION
 *      Handle scall (short call) instructions
 *
 * RETURNS
 *      void
 *
 ************************************************************/

static void do_scall( int addr, unsigned char *buf, int n )
{
    short offset;
    
    offset = ( ( buf[0] & 0x03 ) << 8 ) | buf[1];
    
    if ( buf[0] & 4 )
        offset |= 0xFC00;
    
    operand( "scall   %s", xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr + offset ) );
    xref_addxref( X_CALL, addr - n, addr + offset );
}

/***********************************************************
 *
 * FUNCTION
 *      do_jbc
 *
 * DESCRIPTION
 *      Handle jbc (jump if bit clear) instruction
 *
 * RETURNS
 *      void
 *
 ************************************************************/

static void do_jbc( int addr, unsigned char *buf, int n )
{
    operand( "jbc     R%02X,%d, %s", buf[1], buf[0] & 0x07, xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr + (char)buf[2] ) );
    xref_addxref( X_JMP, addr - n, addr + (char)buf[2] );
}

/***********************************************************
 *
 * FUNCTION
 *      do_jbs
 *
 * DESCRIPTION
 *      Handle jbs (jump if bit set) instruction
 *
 * RETURNS
 *      void
 *
 ************************************************************/

static void do_jbs( int addr, unsigned char *buf, int n )
{
    operand( "jbs     R%02X,%d, %s", buf[1], buf[0] & 0x07, xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr + (char)buf[2] ) );
    xref_addxref( X_JMP, addr - n, addr + (char)buf[2] );
}

/***********************************************************
 *
 * FUNCTION
 *      do_condjump
 *
 * DESCRIPTION
 *      Handle conditional jumps
 *
 * RETURNS
 *      void
 *
 ************************************************************/

static void do_condjmp( int addr, unsigned char *buf, int n )
{
    char *opcodes[] = { "jnst",     "jnh",      "jgt",      "jnc",
                        "jnvt",     "jnv",      "jge",      "jne",
                        "jst",      "jh",       "jle",      "jc", 
                        "jvt",      "jv",       "jlt",      "je" };

    operand( "%-6s  %s", opcodes[buf[0] & 0x0F], xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr + (char)buf[1] ) );
    xref_addxref( X_JMP, addr - n, addr + (char)buf[1] );
}

/***********************************************************
 *
 * FUNCTION
 *      do_f0
 *
 * DESCRIPTION
 *      Handle instructions in the opcode group Fx
 *
 * RETURNS
 *      void
 *
 ************************************************************/

static void do_f0( int addr, unsigned char *buf, int n )
{
    char *opcodes[] = { "ret",      "",         "pushf",    "popf",
                        "pusha",    "popa",     "idlpd",    "trap",
                        "clrc",     "setc",     "di",       "ei",
                        "clrvt",    "nop",      "",         "rst" };

    operand( "%s", opcodes[buf[0] & 0x0F] );
}

/***********************************************************
 *
 * FUNCTION
 *      do_middle
 *
 * DESCRIPTION
 *      Handles 4x .. Bx instruction groups (the middle of the
 *       instruction table).
 *
 * RETURNS
 *      void
 *
 ************************************************************/

static do_middle( int addr, unsigned char *buf, int n, int isSigned )
{
    char *opcodes[] = { "and",      "add",      "sub",      "mul",
                        "and",      "add",      "sub",      "mul",
                        "or",       "xor",      "cmp",      "div",
                        "ld",       "addc",     "subc",     "ldbse" };

    int op;
    int i;
    
    op = 0;
    
        /*
        01.t00..  and     b
        01.t01..  add     b
        01.t10..  sbb     b
        01.t11..  mulu    b
        100t00..  or      b
        100t01..  xor     b
        100t10..  cmp     b
        100t11..  divu    b
        101t00..  ld      b
        101t01..  addc    b
        101t10..  subc    b
        101t11..  ldbse  /ze
        */
    
    if ( buf[0] & 0x80 ) op |= 8;
    if ( buf[0] & 0x20 ) op |= 4;
    if ( buf[0] & 0x08 ) op |= 2;
    if ( buf[0] & 0x04 ) op |= 1;
    
    if ( op == 0x0F )   /* Handle ldb{s|z}e */
        operand( "%s", ( buf[0] & 0x10 ) ? "ldbse " : "ldbze " );
    else
    {
        if ( isSigned )
            operand( "%s%s%c", opcodes[op], 
                    ( isSigned ) ? "" : "u", 
                    ( buf[0] & 0x10 ) ? 'b' : ' ' );
        else
            operand( "%s%c", opcodes[op], ( buf[0] & 0x10 ) ? 'b' : ' ' );
    }
    
    for ( i = strlen( opcodes[op] ); i < 7; i++ )
        operand( " " );;
    
    switch( buf[0] & 0x3 )
    {
        case ADDR_DIRECT:
            if ( n == 3 )
                operand( "R%02X, R%02X", buf[2], buf[1] );
            else
                operand( "R%02X, R%02X, R%02X", buf[3], buf[2], buf[1] );
            break;
            
        case ADDR_IMMED:
            if ( buf[0] & 0x10 || op == 0x0F)
            {
                /* byte const */
                
                if ( n == 4 )
                    operand( "R%02X, ", buf[3] );
                
                operand( "R%02X, #%02X", buf[2], buf[1] );
            }
            else
            {
                /* word const */
                if ( n == 5 )
                    operand( "R%02X, ", buf[4] );
                operand( "R%02X, #%s", buf[3], 
                        xref_genwordaddr( NULL, FORMAT_NUM_16BIT, getAddress(&buf[1]) ) );
                xref_addxref( X_DATA, addr - n, getAddress(&buf[1]) );
            }
            break;

        case ADDR_INDIR:
            if ( n == 4 )
                operand( "R%02X, ", buf[3] );
            
            if ( n >= 3 )
                operand( "R%02X, ", buf[2] );
            
            operand( "[R%02X]", buf[1] & 0xFE );
            if ( buf[1] & 0x01 )
                operand( "+" );

            break;

        case ADDR_INDEX:
            if ( ( buf[0] & 0xE0 ) == 0x40 )
            {
                /* three-op instruction */
                if ( buf[1] & 0x01 )
                {
                    /* word offset */
                    operand( "R%02X, R%02X, %s[R%02X]", buf[5], buf[4], 
                            xref_genwordaddr( NULL, FORMAT_NUM_16BIT, getAddress(&buf[2]) ), buf[1] & 0xFE );
                    xref_addxref( X_PTR, addr - n, getAddress( &buf[2] ) );
                }
                else
                {
                    /* byte offset */
                    operand( "R%02X, R%02X, %02X[R%02X]", buf[4], buf[3], buf[2], buf[1] & 0xFE );
                }
            }
            else
            {
                /* two-op instruction */                
                if ( buf[1] & 0x01 )
                {
                    /* word offset */
                    operand( "R%02X, %s[R%02X]", buf[4], xref_genwordaddr( NULL, FORMAT_NUM_16BIT, getAddress(&buf[2]) ),
                            buf[1] & 0xFE );
                    xref_addxref( X_PTR, addr - n, getAddress(&buf[2]) );                    
                }
                else
                {
                    /* byte offset */
                    operand( "R%02X, %02X[R%02X]", buf[3], buf[2], buf[1] & 0xFE );
                }
            }
            break;
    }
}

/***********************************************************
 *
 * FUNCTION
 *      do_00
 *
 * DESCRIPTION
 *      Handle instructions in the opcode group 0x
 *
 * RETURNS
 *      void
 *
 ************************************************************/

static void do_00( int addr, unsigned char *buf, int n )
{
    char *opcodes[] = { "skip",     "clr",      "not",      "neg",
                        "",         "dec",      "ext",      "inc",
                        "shr",      "shl",      "shra",     "",
                        "shrl",     "shll",     "shral",    "norml",
            
                        "",         "clrb",     "notb",     "negb",
                        "",         "decb",     "extb",     "incb",
                        "shrb",     "shlb",     "shrab",    "",
                        "",         "",         "",         "" };
    
    operand( "%-6s  ", opcodes[buf[0] & 0x1F] );
    
    if ( buf[0] & 0x08 )
    {
        operand( "R%02X, ", buf[2] );
        if ( buf[0] != 0x0F && buf[1] < 0x10 )
            operand( "#%02X", buf[1] );
        else
            operand( "R%02X", buf[1] );
    }
    else
        operand( "R%02X", buf[1] );
}

/***********************************************************
 *
 * FUNCTION
 *      do_c0
 *
 * DESCRIPTION
 *      Handle instructions in the opcode group Cx
 *       store, push and pop
 *
 * RETURNS
 *      void
 *
 ************************************************************/

static void do_c0( int addr, unsigned char *buf, int n )
{
    char *opcodes[] = { "st",       "bmov",     "st",       "st",
                        "stb",      "cmpl",     "stb",      "stb",
                        "push",     "push",     "push",     "push", 
                        "pop",      "",         "pop",      "pop" };

    if ( buf[0] == 0xC1 )
    {
        /* 80196 -- bmov */
        
        operand( "bmov    R%02X, R%02X", buf[1], buf[2] );
    }
    else if ( buf[0] == 0xC5 )
    {
        /* 80196 -- cmpl */
        
        operand( "cmpl    R%02X, R%02X", buf[1], buf[2] );        
    }
    else 
    {
        operand( "%-6s  ", opcodes[buf[0] & 0x0F] );
        
        switch( buf[0] & 0x03 )
        {
            case ADDR_DIRECT:
                if ( n == 3 )
                    operand( "R%02X, ", buf[2] );
                operand( "R%02X", buf[1] );
                break;
                
            case ADDR_IMMED:    /* only PUSH words on to stack */
                operand( "#%s", xref_genwordaddr( NULL, FORMAT_NUM_16BIT, getAddress(&buf[1]) ) );
                break;
                
            case ADDR_INDIR:
                if ( n == 3 )
                    operand( "R%02X, ", buf[2] );
                operand( "[R%02X]", buf[1] & 0xFE );
                if ( buf[1] & 0x01 )
                    operand( "+" );
                break;
                
            case ADDR_INDEX:
                if ( buf[0] & 0x08 )
                {
                    /* push/pop */
                    if ( n == 3 )
                        operand( "%02X[R%02X]", buf[2], buf[1] & 0xFE );
                    else
                    {
                        operand( "%s[R%02X]", xref_genwordaddr( NULL, FORMAT_NUM_16BIT, getAddress(&buf[2]) ), buf[1] & 0xFE );
                        xref_addxref( X_PTR, addr - n, getAddress(&buf[2]) );
                    }
                }
                else
                {
                    /* st(b) */
                    if ( n == 4 )
                        operand( "R%02X, %02X[R%02X]", buf[3], buf[2], buf[1] & 0xFE );
                    else
                    {
                        operand( "R%02X, %s[R%02X]", buf[4], xref_genwordaddr( NULL, FORMAT_NUM_16BIT, getAddress(&buf[2]) ),
                                buf[1] & 0xFE);
                        xref_addxref( X_PTR, addr - n, getAddress(&buf[2]) );
                    }
                }
                break;
        }
    }
}

/***********************************************************
 *
 * FUNCTION
 *      do_e0
 *
 * DESCRIPTION
 *      Handle instructions in the opcode group Ex
 *       djnz, br, ljmp, lcall
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
#define OP_DJNZ     0xE0
#define OP_DJNZW    0xE1
#define OP_BR       0xE3
#define OP_LJMP     0xE7
#define OP_LCALL    0xEF

static void do_e0( int addr, unsigned char *buf, int n )
{
    switch(buf[0])
    {
        case OP_DJNZ:
            operand( "djnz    R%02X, %s", buf[1], xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr + (char)buf[2] ) );
            xref_addxref( X_JMP, addr - n, addr + (char)buf[2] );
            break;

        case OP_DJNZW:
            /* 80196 */
            operand( "djnzw   R%02X, %s", buf[1], xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr + (char)buf[2] ) );
            xref_addxref( X_JMP, addr - n, addr + (char)buf[2] );
            break;
            
        case OP_BR:
            operand( "br      [R%02X]", buf[1] );
            break;
            
        case OP_LJMP:
            operand( "ljmp    %s", xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr + getOffset(buf + 1) ) );
            xref_addxref( X_JMP, addr - n, addr + getOffset(buf + 1) );
            break;
        
        case OP_LCALL:
            operand( "lcall   %s", xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr + getOffset(buf + 1) ) );
            xref_addxref( X_CALL, addr - n, addr + getOffset(buf + 1) );
            break;
        
        default:
            operand("???");
    }
}

/*****************************************************************************
 *        Public Functions
 *****************************************************************************/

/***********************************************************
 *
 * FUNCTION
 *      dasm_insn
 *
 * DESCRIPTION
 *      Disassembles the next instruction in the input stream.
 *
 * RETURNS
 *      address of next input byte
 *
 ************************************************************/
ADDR dasm_insn( FILE *f, char * outbuf, ADDR addr )
{
	int isSigned = 0;
	int opc;
	int n;
	unsigned char buf[8];
	int i;
	
	output_buffer = outbuf;
            
   opc = next( f, &addr );
   if ( opc == 0xFE )
   {
      isSigned = 1;
      opc = next( f, &addr );
   }

   n = instrlen[opc];
   buf[0] = opc;
            
   if ( n < 0 )
   {
      n = -n;
      buf[1] = next( f, &addr );
      if ( buf[1] & 1 ) 
         n++;
      for ( i = 2; i < n; i++ )
         buf[i] = next( f, &addr );
   }
   else
      for ( i = 1; i < n; i++ )
         buf[i] = next( f, &addr );

   if ( n == 0 )
   {
      /* Unknown instruction */
      operand( "???" );
   }
	else
	{

            if      ( ( opc & 0xf8 ) == 0x20 )  do_sjmp   ( addr, buf, n );
            else if ( ( opc & 0xf8 ) == 0x28 )  do_scall  ( addr, buf, n );
            else if ( ( opc & 0xf8 ) == 0x30 )  do_jbc    ( addr, buf, n );
            else if ( ( opc & 0xf8 ) == 0x38 )  do_jbs    ( addr, buf, n );
            else if ( ( opc & 0xf0 ) == 0xd0 )  do_condjmp( addr, buf, n );
            else if ( ( opc & 0xf0 ) == 0xf0 )  do_f0     ( addr, buf, n );
            else if ( ( opc & 0xf0 ) == 0xe0 )  do_e0     ( addr, buf, n );
            else if ( ( opc & 0xf0 ) == 0xc0 )  do_c0     ( addr, buf, n );
            else if ( ( opc & 0xe0 ) == 0 )     do_00     ( addr, buf, n );
            else                                do_middle ( addr, buf, n, isSigned );
	}

   return addr;
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
