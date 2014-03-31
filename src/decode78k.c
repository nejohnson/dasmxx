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



char * dasm_name = "dasm78k";
char * dasm_description = "NEC 78k";





/* Tables of register names used in addressing modes */

static const char * MEM_MOD_RI[8] = {
	"[DE+]",
	"[HL+]",
	"[DE-]",
	"[HL-]",
	"[DE]",
	"[HL]",
	"[VP]",
	"[UP]"
};

static const char * MEM_MOD_BI[6] = {
	"[DE+A]",
	"[HL+A]",
	"[DE+B]",
	"[HL+B]",
	"[VP+DE]",
	"[VP+HL]"
};

static const char * MEM_MOD_BASE[5] = {
	"[DE+",
	"[SP+",
	"[HL+",
	"[UP+",
	"[VP+"
};

static const char * MEM_MOD_INDEX[4] = {
	"[DE]",
	"[A]",
	"[HL]",
	"[B]"
};

static const char * RP[8] = {
	"RP0",
	"RP1",
	"RP2",
	"RP3",
	"RP4",
	"RP5",
	"RP6",
	"RP7"
};

static const char * RP1[8] = {
	"RP0",
	"RP4",
	"RP1",
	"RP5",
	"RP2",
	"RP6",
	"RP3",
	"RP7"
};

static const char * RP2[4] = {
	"VP", "UP", "DE", "HL"
};

static const char * R2[2] = {
	"C", "B"
};

#define SADDR_OFFSET		( 0xFE20 )
#define SFR_OFFSET			( 0xFF00 )


/* Common output formats */
#define FORMAT_NUM_8BIT		"$%02X"
#define FORMAT_NUM_16BIT		"$%04X"
#define FORMAT_REG				"R%d"

#define BIT(n)				( 1 << (n) )

/* Construct a 16-bit word out of low and high bytes */
#define MK_WORD(l,h)		( ((l) & 0xFF) | (((h) & 0xFF) << 8) )



static char * output_buffer = NULL;



/*****************************************************************************
 *        Private Functions
 *****************************************************************************/

/***********************************************************
 *
 ***********************************************************/


/*------------------------------------------------------------------------------*/
#if 0

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

static int do_sjmp( int addr, unsigned char *buf, int n )
{
   int column = 0;
    short offset;
    
    offset = ( ( buf[0] & 0x03 ) << 8 ) | buf[1];
    
    if ( buf[0] & 4 )
        offset |= 0xFC00;
    
    columprintf( "sjmp    %s", emitwordaddr( addr + offset ) );
    xref_addxref( X_JMP, addr - n, addr + offset );
	 
	 return column;
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

static int do_scall( int addr, unsigned char *buf, int n )
{
   int column = 0;
    short offset;
    
    offset = ( ( buf[0] & 0x03 ) << 8 ) | buf[1];
    
    if ( buf[0] & 4 )
        offset |= 0xFC00;
    
    columprintf( "scall   %s", emitwordaddr( addr + offset ) );
    xref_addxref( X_CALL, addr - n, addr + offset );
	 
	 return column;
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

static int do_jbc( int addr, unsigned char *buf, int n )
{
   int column = 0;
    columprintf( "jbc     R%02X,%d, %s", buf[1], buf[0] & 0x07, emitwordaddr( addr + (char)buf[2] ) );
    xref_addxref( X_JMP, addr - n, addr + (char)buf[2] );
	 return column;
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

static int do_jbs( int addr, unsigned char *buf, int n )
{
   int column = 0;
    columprintf( "jbs     R%02X,%d, %s", buf[1], buf[0] & 0x07, emitwordaddr( addr + (char)buf[2] ) );
    xref_addxref( X_JMP, addr - n, addr + (char)buf[2] );
	 return column;
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

static int do_condjmp( int addr, unsigned char *buf, int n )
{
   int column = 0;
    char *opcodes[] = { "jnst",     "jnh",      "jgt",      "jnc",
                        "jnvt",     "jnv",      "jge",      "jne",
                        "jst",      "jh",       "jle",      "jc", 
                        "jvt",      "jv",       "jlt",      "je" };

    columprintf( "%-6s  %s", opcodes[buf[0] & 0x0F], emitwordaddr( addr + (char)buf[1] ) );
    xref_addxref( X_JMP, addr - n, addr + (char)buf[1] );
	 
	 return column;
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

static int do_f0( int addr, unsigned char *buf, int n )
{
   int column = 0;
    char *opcodes[] = { "ret",      "",         "pushf",    "popf",
                        "pusha",    "popa",     "idlpd",    "trap",
                        "clrc",     "setc",     "di",       "ei",
                        "clrvt",    "nop",      "",         "rst" };

    columprintf( "%s", opcodes[buf[0] & 0x0F] );
	 
	return column;
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

static int do_middle( int addr, unsigned char *buf, int n, int isSigned )
{
   int column = 0;
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
        columprintf( "%s", ( buf[0] & 0x10 ) ? "ldbse " : "ldbze " );
    else
    {
        if ( isSigned )
            columprintf( "%s%s%c", opcodes[op], 
                    ( isSigned ) ? "" : "u", 
                    ( buf[0] & 0x10 ) ? 'b' : ' ' );
        else
            columprintf( "%s%c", opcodes[op], ( buf[0] & 0x10 ) ? 'b' : ' ' );
    }
    
    for ( i = strlen( opcodes[op] ); i < 7; i++ )
        putchar( ' ' ), column++;
    
    switch( buf[0] & 0x3 )
    {
        case ADDR_DIRECT:
            if ( n == 3 )
                columprintf( "R%02X, R%02X", buf[2], buf[1] );
            else
                columprintf( "R%02X, R%02X, R%02X", buf[3], buf[2], buf[1] );
            break;
            
        case ADDR_IMMED:
            if ( buf[0] & 0x10 || op == 0x0F)
            {
                /* byte const */
                
                if ( n == 4 )
                    columprintf( "R%02X, ", buf[3] );
                
                columprintf( "R%02X, #%02X", buf[2], buf[1] );
            }
            else
            {
                /* word const */
                if ( n == 5 )
                    columprintf( "R%02X, ", buf[4] );
                columprintf( "R%02X, #%s", buf[3], 
                        emitwordaddr( getaddress(&buf[1]) ) );
                xref_addxref( X_DATA, addr - n, getaddress(&buf[1]) );
            }
            break;

        case ADDR_INDIR:
            if ( n == 4 )
                columprintf( "R%02X, ", buf[3] );
            
            if ( n >= 3 )
                columprintf( "R%02X, ", buf[2] );
            
            columprintf( "[R%02X]", buf[1] & 0xFE );
            if ( buf[1] & 0x01 )
                putchar( '+' ), column++;

            break;

        case ADDR_INDEX:
            if ( ( buf[0] & 0xE0 ) == 0x40 )
            {
                /* three-op instruction */
                if ( buf[1] & 0x01 )
                {
                    /* word offset */
                    columprintf( "R%02X, R%02X, %s[R%02X]", buf[5], buf[4], 
                            emitwordaddr( getaddress(&buf[2]) ), buf[1] & 0xFE );
                    xref_addxref( X_PTR, addr - n, getaddress( &buf[2] ) );
                }
                else
                {
                    /* byte offset */
                    columprintf( "R%02X, R%02X, %02X[R%02X]", buf[4], buf[3], buf[2], buf[1] & 0xFE );
                }
            }
            else
            {
                /* two-op instruction */                
                if ( buf[1] & 0x01 )
                {
                    /* word offset */
                    columprintf( "R%02X, %s[R%02X]", buf[4], emitwordaddr( getaddress(&buf[2]) ),
                            buf[1] & 0xFE );
                    xref_addxref( X_PTR, addr - n, getaddress(&buf[2]) );                    
                }
                else
                {
                    /* byte offset */
                    columprintf( "R%02X, %02X[R%02X]", buf[3], buf[2], buf[1] & 0xFE );
                }
            }
            break;
    }
	 
	 return column;
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

static int do_00( int addr, unsigned char *buf, int n )
{
   int column = 0;
    char *opcodes[] = { "skip",     "clr",      "not",      "neg",
                        "",         "dec",      "ext",      "inc",
                        "shr",      "shl",      "shra",     "",
                        "shrl",     "shll",     "shral",    "norml",
            
                        "",         "clrb",     "notb",     "negb",
                        "",         "decb",     "extb",     "incb",
                        "shrb",     "shlb",     "shrab",    "",
                        "",         "",         "",         "" };
    
    columprintf( "%-6s  ", opcodes[buf[0] & 0x1F] );
    
    if ( buf[0] & 0x08 )
    {
        columprintf( "R%02X, ", buf[2] );
        if ( buf[0] != 0x0F && buf[1] < 0x10 )
            columprintf( "#%02X", buf[1] );
        else
            columprintf( "R%02X", buf[1] );
    }
    else
        columprintf( "R%02X", buf[1] );
		  
	return column;
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

static int do_c0( int addr, unsigned char *buf, int n )
{
   int column = 0;
    char *opcodes[] = { "st",       "bmov",     "st",       "st",
                        "stb",      "cmpl",     "stb",      "stb",
                        "push",     "push",     "push",     "push", 
                        "pop",      "",         "pop",      "pop" };

    if ( buf[0] == 0xC1 )
    {
        /* 80196 -- bmov */
        
        columprintf( "bmov    R%02X, R%02X", buf[1], buf[2] );
    }
    else if ( buf[0] == 0xC5 )
    {
        /* 80196 -- cmpl */
        
        columprintf( "cmpl    R%02X, R%02X", buf[1], buf[2] );        
    }
    else 
    {
        columprintf( "%-6s  ", opcodes[buf[0] & 0x0F] );
        
        switch( buf[0] & 0x03 )
        {
            case ADDR_DIRECT:
                if ( n == 3 )
                    columprintf( "R%02X, ", buf[2] );
                columprintf( "R%02X", buf[1] );
                break;
                
            case ADDR_IMMED:    /* only PUSH words on to stack */
                columprintf( "#%s", emitwordaddr( getaddress(&buf[1]) ) );
                break;
                
            case ADDR_INDIR:
                if ( n == 3 )
                    columprintf( "R%02X, ", buf[2] );
                columprintf( "[R%02X]", buf[1] & 0xFE );
                if ( buf[1] & 0x01 )
                    putchar( '+' ), column++;
                break;
                
            case ADDR_INDEX:
                if ( buf[0] & 0x08 )
                {
                    /* push/pop */
                    if ( n == 3 )
                        columprintf( "%02X[R%02X]", buf[2], buf[1] & 0xFE );
                    else
                    {
                        columprintf( "%s[R%02X]", emitwordaddr( getaddress(&buf[2]) ), buf[1] & 0xFE );
                        xref_addxref( X_PTR, addr - n, getaddress(&buf[2]) );
                    }
                }
                else
                {
                    /* st(b) */
                    if ( n == 4 )
                        columprintf( "R%02X, %02X[R%02X]", buf[3], buf[2], buf[1] & 0xFE );
                    else
                    {
                        columprintf( "R%02X, %s[R%02X]", buf[4], emitwordaddr( getaddress(&buf[2]) ),
                                buf[1] & 0xFE);
                        xref_addxref( X_PTR, addr - n, getaddress(&buf[2]) );
                    }
                }
                break;
        }
    }
	 
	 return column;
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

static int do_e0( int addr, unsigned char *buf, int n )
{
   int column = 0;
    switch(buf[0])
    {
        case OP_DJNZ:
            columprintf( "djnz    R%02X, %s", buf[1], emitwordaddr( addr + (char)buf[2] ) );
            xref_addxref( X_JMP, addr - n, addr + (char)buf[2] );
            break;

        case OP_DJNZW:
            /* 80196 */
            columprintf( "djnzw   R%02X, %s", buf[1], emitwordaddr( addr + (char)buf[2] ) );
            xref_addxref( X_JMP, addr - n, addr + (char)buf[2] );
            break;
            
        case OP_BR:
            columprintf( "br      [R%02X]", buf[1] );
            break;
            
        case OP_LJMP:
            columprintf( "ljmp    %s", emitwordaddr( addr + getoffset(buf + 1) ) );
            xref_addxref( X_JMP, addr - n, addr + getoffset(buf + 1) );
            break;
        
        case OP_LCALL:
            columprintf( "lcall   %s", emitwordaddr( addr + getoffset(buf + 1) ) );
            xref_addxref( X_CALL, addr - n, addr + getoffset(buf + 1) );
            break;
        
        default:
            columprintf("???");
    }
	 
	 return column;
}
#endif

/*------------------------------------------------------------------------------*/


static void opcode( const char *opcode )
{
	int n = sprintf( output_buffer, "%-9s", opcode );
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







static void arith_operands2( int opc, int opc2 )
{
	if ( ( opc & 0xF0 ) == 0xA0 )
	{
		operand( "A,#" FORMAT_NUM_8BIT, opc2 );
	}
	else if ( ( opc & 0xF0 ) == 0x80 )
	{
		if ( opc2 & (1 << 3) )
			operand( "%s,%s", RP[opc >> 4], RP1[opc2 & 0x07] );
		else
			operand( FORMAT_REG "," FORMAT_REG, opc2 >> 4, (opc & 0x07) );
	}
	else if ( ( opc & 0xF0 ) == 0x90 )
	{
		operand( "A,%s", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ) );	
	}
}


static void do_05( int opc2 )
{
	int op = opc2 >> 3;
	int arg = opc2 & 0x07;
	
	switch ( op )
	{
case 0x01: case 0x03:
		opcode( op == 0x01 ? "mulu" : "divuw" );
		operand( FORMAT_REG, arg );
		break;
		
case 0x05: case 0x1D:
		opcode( op == 0x05 ? "muluw" : "divux" );
		operand( RP1[arg] );
		break;
		
case 0x09: case 0x0D:
		opcode( "br" );
		operand( op == 0x09 ? "[%s]" : "%s", RP1[arg] );
		break;
		
case 0x1B:
		opcode( "brkcs" );
		operand( "RB%d", arg );
		break;
		
case 0x15: case 0x17:
		opcode( "sel" );
		operand( "RB%d%s", arg, op == 0x17 ? ",ALT" : "" );
		break;
		
case 0x11: case 0x13:
		opcode( op == 0x11 ? "ror4" : "rol4" );
		operand( "[%s]", RP1[arg] );
		break;

case 0x0B: case 0x0F:
		opcode( "call" );
		operand( op == 0x0B ? "%s" : "[%s]", RP1[arg] );
		break;	
	}
}


static void do_shift_rot( int opc, int opc2 )
{
	int op = ( ( opc & 0x01 ) << 2 ) | ( ( opc2 & 0xC0 ) >> 6 );
	int reg = opc2 & 0x07;
	int shift = ( opc2 >> 3 ) & 0x07;
	
	static char * optab[8] = {
		"rorc",
		"ror",
		"shr",
		"shrw",
		"rolc",
		"rol",
		"shl",
		"shlw"
	};
	
	opcode( optab[op] );
	if ( ( opc2 & 0xC0 ) == 0xC0 )
		operand( "%s,%d", RP1[reg], shift );
	else
		operand( FORMAT_REG ",%d", reg, shift );
}



static void do_bit_ops2( int opc, int opc2 )
{
	int bit = opc2 & 0x07;
	int op = opc2 >> 4;
	int args = ( ( opc & 0x03 ) << 1 ) | ( ( opc2 >> 3 ) & 0x01 );
	
	switch ( op )
	{
case 0x00: case 0x01: opcode( "mov1" ); break;
case 0x02: case 0x03: opcode( "and1" ); break;
case 0x04: case 0x05: opcode( "or1"  ); break;
case 0x06:            opcode( "xor1" ); break;
case 0x08:            opcode( "set1" ); break;
case 0x09:            opcode( "clr1" ); break;
case 0x07:            opcode( "not1" ); break;
default: return;
	}
	
	if ( ( op < 0x07 ) && !(op & 0x01 ) )
		operand( "CY," );
	
	switch ( args )
	{
case 0x07:	operand( "A.%d", bit ); break;
case 0x06:  operand( "X.%d", bit ); break;
case 0x05:  operand( "PSWL.%d", bit ); break;
case 0x04:  operand( "PWSH.%d", bit ); break;
	}
	
	if ( ( op < 0x07 ) && (op & 0x01 ) )
		operand( ",CY" );
}


static void do_push_pop( int opc, int opc2 )
{
	int bit;
	int comma = 0;
	
	switch ( opc )
	{
case 0x35: opcode( "push" ); break;
case 0x37: opcode( "pushu" ); break;
case 0x34: opcode( "pop" ); break;
case 0x36: opcode( "popu" ); break;
	}
	
	for ( bit = 0; bit < 8; bit++ )
	{
		if ( opc2 & BIT(bit) )
		{
			if ( comma )
				operand( "," );
			operand( RP[bit] );
			comma = 1;
		}
	}
}



static void do_bxx2( int opc, int opc2, int addr )
{
	int op = opc & 0x07;
	static char *optab[] = {
		"bne",	"be",
		"bnc",	"bc",
		"bnv",   "bv",
		"bp",		"bn"
	};
	
	addr += (signed char)opc2;
	
	opcode( optab[op] );
	operand( "%s", xref_genwordaddr( NULL, "$", addr ) );
}



static void do_string_ops( int opc, int opc2 )
{
	int dest = opc2 & BIT(4);
	int src  = opc2 & BIT(5);
	int op   = opc2 & 0x07;
	
	switch ( op )
	{
case 0x00: opcode( src ? "movbk" : "movm" ); break;
case 0x01: opcode( src ? "xchbk" : "xchm" ); break;
case 0x04: opcode( src ? "cmpbke" : "cmpme" ); break;
case 0x05: opcode( src ? "cmpbkne" : "cmpmne" ); break;	
case 0x07: opcode( src ? "cmpbkc" : "cmpmc" ); break;
case 0x06: opcode( src ? "cmpbknc" : "cmpmnc" ); break;
default: return;
	}
	
	operand( "%s,%s",
					dest ? "[DE-]" : "[DE+]", 
					src ? ( dest ? "[HL-]" : "[HL+]" ) : "A" );
}



static void do_01_3( int opc2, int opc3 )
{
	int is_word = 0;
	
	switch ( opc2 )
	{
case 0x21: opcode( "xch" ); break;
case 0x1B: opcode( "xchw" ); is_word = 1; break;
case 0x98: opcode( "add" ); break;
case 0x99: opcode( "addc" ); break;
case 0x9A: opcode( "sub" ); break;
case 0x9B: opcode( "subc" ); break;
case 0x9C: opcode( "and" ); break;
case 0x9E: opcode( "or" ); break;
case 0x9D: opcode( "xor" ); break;
case 0x9F: opcode( "cmp" ); break;
case 0x1D: opcode( "addw" ); is_word = 1; break;
case 0x1E: opcode( "subw" ); is_word = 1; break;
case 0x1F: opcode( "cmpw" ); is_word = 1; break;
default: return;
	}
	
	operand( "%s,%s", is_word ? "AX" : "A", xref_genwordaddr( NULL, "$", opc3 + SFR_OFFSET ) );
}


static void do_08_3( int opc2, int opc3 )
{
	int bit = opc2 & 0x07;
	int op  = opc2 >> 3;
	
	switch ( op )
	{
case 0x00:
		opcode( "mov1" );
		operand( "CY,%s.%d", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), bit );
		break;
		
case 0x01:
		opcode( "mov1" );
		operand( "CY,%s.%d", xref_genwordaddr( NULL, "$", opc2 + SFR_OFFSET ), bit );
		break;
	
case 0x02:
		opcode( "mov1" );
		operand( "%s.%d,CY", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), bit );
		break;
		
case 0x03:
		opcode( "mov1" );
		operand( "%s.%d,CY", xref_genwordaddr( NULL, "$", opc2 + SFR_OFFSET ), bit );
		break;
	
case 0x04:
		opcode( "and1" );
		operand( "CY,%s.%d", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), bit );
		break;
		
case 0x06:
		opcode( "and1" );
		operand( "CY,/%s.%d", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), bit );
		break;
	
case 0x05:
		opcode( "and1" );
		operand( "CY,%s.%d", xref_genwordaddr( NULL, "$", opc2 + SFR_OFFSET ), bit );
		break;
		
case 0x07:
		opcode( "and1" );
		operand( "CY,/%s.%d", xref_genwordaddr( NULL, "$", opc2 + SFR_OFFSET ), bit );
		break;
	
case 0x08:
		opcode( "or1" );
		operand( "CY,%s.%d", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), bit );
		break;
		
case 0x0A:
		opcode( "or1" );
		operand( "CY,/%s.%d", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), bit );
		break;
	
case 0x09:
		opcode( "or1" );
		operand( "CY,%s.%d", xref_genwordaddr( NULL, "$", opc2 + SFR_OFFSET ), bit );
		break;
		
case 0x0B:
		opcode( "or1" );
		operand( "CY,/%s.%d", xref_genwordaddr( NULL, "$", opc2 + SFR_OFFSET ), bit );
		break;
	
case 0x0C:
		opcode( "xor1" );
		operand( "CY,%s.%d", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), bit );
		break;
		
case 0x0D:
		opcode( "xor1" );
		operand( "CY,%s.%d", xref_genwordaddr( NULL, "$", opc2 + SFR_OFFSET ), bit );
		break;
		
case 0x11:
		opcode( "set1" );
		operand( "%s.%d", xref_genwordaddr( NULL, "$", opc2 + SFR_OFFSET ), bit );
		break;
		
case 0x13:
		opcode( "clr1" );
		operand( "%s.%d", xref_genwordaddr( NULL, "$", opc2 + SFR_OFFSET ), bit );
		break;
		
case 0x0E:
		opcode( "not1" );
		operand( "%s.%d", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), bit );
		break;
		
case 0x0F:
		opcode( "not1" );
		operand( "%s.%d", xref_genwordaddr( NULL, "$", opc2 + SFR_OFFSET ), bit );
		break;	
	
default: return;
	}	
}



static void do_07_3( int opc2, int opc3, int addr )
{
	switch ( opc2 )
	{
case 0xFB: opcode( "bgt" ); break;
case 0xF9: opcode( "bge" ); break;
case 0xF8: opcode( "blt" ); break;
case 0xFE: opcode( "ble" ); break;
case 0xFD: opcode( "bh" ); break;
case 0xFC: opcode( "bnh" ); break;

default: return;
	}
	
	operand( xref_genwordaddr( NULL, "$", addr + (signed char)opc3 ) );	
}



static void do_bxx_3( int opc, int opc2, int opc3, int addr )
{
	int bit = opc2 & 0x07;
	int addr16 = addr + (signed char)opc3;
	int op = (opc << 8) | (opc2 >> 3);
	
	switch ( op )
	{
case 0x317: opcode( "bt" ); operand( "A.%d", bit ); break;
case 0x316: opcode( "bt" ); operand( "X.%d", bit ); break;
case 0x217: opcode( "bt" ); operand( "PSWH.%d", bit ); break;
case 0x216: opcode( "bt" ); operand( "PSWL.%d", bit ); break;

case 0x315: opcode( "bf" ); operand( "A.%d", bit ); break;
case 0x314: opcode( "bf" ); operand( "X.%d", bit ); break;
case 0x215: opcode( "bf" ); operand( "PSWH.%d", bit ); break;
case 0x214: opcode( "bf" ); operand( "PSWL.%d", bit ); break;

case 0x31B: opcode( "btclr" ); operand( "A.%d", bit ); break;
case 0x31A: opcode( "btclr" ); operand( "X.%d", bit ); break;
case 0x21B: opcode( "btclr" ); operand( "PSWH.%d", bit ); break;
case 0x21A: opcode( "btclr" ); operand( "PSWL.%d", bit ); break;

case 0x319: opcode( "bfset" ); operand( "A.%d", bit ); break;
case 0x318: opcode( "bfset" ); operand( "X.%d", bit ); break;
case 0x219: opcode( "bfset" ); operand( "PSWH.%d", bit ); break;
case 0x218: opcode( "bfset" ); operand( "PSWL.%d", bit ); break;

default: return;
	}	

	operand( ",%s", xref_genwordaddr( NULL, "$", addr16 ) );	
}


static void do_memmod_ops_4( int opc, int opc2, int opc3, int opc4 )
{
	int mod = opc & 0x1F;
	int mem = (opc2 >> 4) & 0x07;
	int dir = opc2 & 0x80;
	int op  = opc2 & 0x0F;
	int offset = MK_WORD( opc3, opc4 );
	
	static const char * optab[16] = {
		"mov",
		NULL,
		NULL,
		NULL,
		"xch",
		NULL,
		NULL,
		NULL,
		"add",
		"addc",
		"sub",
		"subc",
		"and",
		"xor",
		"or",
		"cmp"
	};
	
	if ( optab[op] == NULL )
		return;
	
	opcode( optab[op] );
	
	if ( !dir )
		operand( "A," );
		
	if ( mod == 0x16 )
		operand( MEM_MOD_RI[mem] );
	else if ( mod == 0x17 )
		operand( MEM_MOD_BI[mem] );
	else if ( mod == 0x06 )
		operand( "%s" FORMAT_NUM_8BIT "]", MEM_MOD_BASE[mem], opc3 );
	else if ( mod == 0x0A )
		operand( FORMAT_NUM_16BIT "%s", offset, MEM_MOD_INDEX[mem] );
	else
		return;	
		
	if ( dir )
		operand( ",A" );	
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
 *      f - file stream to read (pass to calls to next() )
 *      addr - address of first input byte for this insn
 *      pchars - write here the number of characters emitted
 *
 * RETURNS
 *      address of next input byte
 *
 ************************************************************/
 
int dasm_insn( FILE *f, char *outbuf, int addr )
{
	int opc, opc2, opc3, opc4, opc5;
	
	output_buffer = outbuf;

	opc = next( f, &addr );
	
	/* Catch all 1-byte operations here rather than polute groups */
	switch( opc )
	{
case 0xD0: case 0xD1: case 0xD2: case 0xD3:
case 0xD4: case 0xD5: case 0xD6: case 0xD7:
case 0xD8: case 0xD9: case 0xDA: case 0xDB:
case 0xDC: case 0xDD: case 0xDE: case 0xDF:
		opcode( opc & 0x08? "xch" : "mov" );
		operand( "A," FORMAT_REG, opc & 0x07 );
		break;
		
case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55:
case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D:
		opcode( "mov" );
		operand( "%s%s%s", 
				opc & 0x08 ? "A," : "",
				MEM_MOD_RI[opc & 0x07],
				opc & 0x08 ? "" : ",A" );
		break;

case 0xC0: case 0xC1: case 0xC2: case 0xC3:
case 0xC4: case 0xC5: case 0xC6: case 0xC7:
case 0xC8: case 0xC9: case 0xCA: case 0xCB:
case 0xCC: case 0xCD: case 0xCE: case 0xCF:
		opcode( opc & 0x08 ? "dec" : "inc" );
		operand( FORMAT_REG, opc & 0x07 );
		break;
		
case 0x44: case 0x45: case 0x46: case 0x47:
case 0x4C: case 0x4D: case 0x4E: case 0x4F:
		opcode( opc & 0x08 ? "decw" : "incw" );
		operand( RP2[opc & 0x03] );
		break;
		
case 0x04:
		opcode( "adj4" );
		break;
		
case 0x41:
		opcode( "set1" ); operand( "CY" );
		break;
		
case 0x40:
		opcode( "clr1" ); operand( "CY" );
		break;
		
case 0x42:
		opcode( "not1" ); operand( "CY" );
		break;
		
case 0xE0: case 0xE1: case 0xE2: case 0xE3:
case 0xE4: case 0xE5: case 0xE6: case 0xE7:
case 0xE8: case 0xE9: case 0xEA: case 0xEB:
case 0xEC: case 0xED: case 0xEE: case 0xEF:
case 0xF0: case 0xF1: case 0xF2: case 0xF3:
case 0xF4: case 0xF5: case 0xF6: case 0xF7:
case 0xF8: case 0xF9: case 0xFA: case 0xFB:
case 0xFC: case 0xFD: case 0xFE: case 0xFF:
		opcode( "callt" );
		operand( "[" FORMAT_NUM_8BIT "]", opc & 0x1F );
		break;
		
case 0x5E:
		opcode( "brk" );
		break;
		
case 0x56:
		opcode( "ret" );
		break;
		
case 0x57:
		opcode( "reti" );
		break;
		
case 0x49:
		opcode( "push" );
		operand( "PSW" );
		break;
		
case 0x48:
		opcode( "pop" );
		operand( "PSW" );
		break;
		
case 0x43:
		opcode( "swrs" );
		break;
		
case 0x00:
		opcode( "nop" );
		break;
		
case 0x4B:
		opcode( "ei" );
		break;
		
case 0x4A:
		opcode( "di" );
		break;
	}
	
	if ( output_buffer != outbuf )
	{
		return addr;
	}
	
	/* 2-byte Operations 2222222222222222222222222222222222222222222222222222222 */
	opc2 = next( f, &addr );
	switch( opc )
	{
case 0xB8: case 0xB9: case 0xBA: case 0xBB:
case 0xBC: case 0xBD: case 0xBE: case 0xBF:
		opcode( "mov" );
		operand( FORMAT_REG ",#" FORMAT_NUM_8BIT, opc & 0x07, opc2 );
		break;
		
case 0x24: case 0x25:
		if ( opc2 & (1 << 3) )
		{
			/* Word */
			opcode( opc == 0x24 ? "movw" : "xchw" );
			operand( "%s,%s", RP[opc2 >> 5], RP1[opc2 & 0x07] );
		}
		else
		{
			/* Byte */
			opcode( opc == 0x24 ? "mov" : "xch" );
			operand( FORMAT_REG "," FORMAT_REG, opc2 >> 4, opc2 & 0x07 );
		}
		break;
		
case 0x20: case 0x22:
		opcode( "mov" );
		operand( opc & 0x02 ? "%s,A" : "A,%s", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ) );
		break;
		
case 0x10: case 0x12:
		opcode( "mov" );
		if ( !( opc & 0x02 ) )
			operand( "A," );
		
		if ( opc2 == 0xFE )
			operand( "PSWL" );
		else if ( opc2 == 0xFF )
			operand( "PSWH" );
		else
			operand( "%s", xref_genwordaddr( NULL, "$", opc2 + SFR_OFFSET ) );
		
		if ( opc & 0x02 )
			operand( ",A" );
		break;
		
case 0x18: case 0x19:
		opcode( "mov" );
		if ( !( opc & 0x01 ) )
			operand( "A," );
		
		operand( "[%s]", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ) );
		
		if ( opc & 0x02 )
			operand( ",A" );
		break;

case 0x21:
		opcode( "xch" );
		operand( "%s", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ) );
		break;

case 0x23:
		opcode( "xch" );
		operand( "[%s]", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ) );
		break;
	
case 0x1A: case 0x1C:
		opcode( "movw" );
		operand( opc == 0x1A ? "%s,AX" : "AX,%s", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ) );
		break;	
	
case 0x11: case 0x13:
		opcode( "movw" );
		if ( !( opc & 0x02 ) )
			operand( "AX," );
		
		if ( opc2 == 0xFC )
			operand( "SP" );
		else
			operand( "%s", xref_genwordaddr( NULL, "$", opc2 + SFR_OFFSET ) );
		
		if ( opc & 0x02 )
			operand( ",XA" );
		break;	
	
case 0x1B:
		opcode( "xchw" );
		operand( "AX,%s", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ) );
		break;	

case 0xA8: case 0x88: case 0x98:
		opcode( opc & (1<<3) ? "addw" : "add" );
		arith_operands2( opc, opc2 );
		break;
		
case 0xA9: case 0x89: case 0x99:
		opcode( "addc" );
		arith_operands2( opc, opc2 );
		break;		

case 0xAA: case 0x8A: case 0x9A:
		opcode( opc & (1<<3) ? "subw" : "sub" );
		arith_operands2( opc, opc2 );
		break;
		
case 0xAB: case 0x8B: case 0x9B:
		opcode( "subc" );
		arith_operands2( opc, opc2 );
		break;		
		
case 0xAC: case 0x8C: case 0x9C:
		opcode( "and" );
		arith_operands2( opc, opc2 );
		break;
		
case 0xAE: case 0x8E: case 0x9E:
		opcode( "or" );
		arith_operands2( opc, opc2 );
		break;		

case 0xAD: case 0x8D: case 0x9D:
		opcode( "xor" );
		arith_operands2( opc, opc2 );
		break;
		
case 0xAF: case 0x8F: case 0x9F:
		opcode( opc & (1<<3) ? "cmpw" : "cmp" );
		arith_operands2( opc, opc2 );
		break;		
		
case 0x1D: case 0x1E: case 0x1F:
		opcode( opc == 0x1D ? "addw" : opc == 0x1E ? "subw" : "cmpw" );
		operand( "AX,%s", xref_genwordaddr( NULL, "$", opc2 + SFR_OFFSET ) );
		break;
		
case 0x05:
		do_05( opc2 );
		break;	

case 0x26: case 0x27:
		opcode( opc == 0x26 ? "inc" : "dec" );
		operand( xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ) );
		break;

case 0x30: case 0x31:
		do_shift_rot( opc, opc2 );
		break;

case 0xA0: case 0xA1: case 0xA2: case 0xA3:
case 0xA4: case 0xA5: case 0xA6: case 0xA7:
		opcode( "clr1" );
		operand( "%s.%d", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), opc & 0x07 );
		break;
		
case 0xB0: case 0xB1: case 0xB2: case 0xB3:
case 0xB4: case 0xB5: case 0xB6: case 0xB7:
		opcode( "set1" );
		operand( "%s.%d", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), opc & 0x07 );
		break;
		
case 0x02: case 0x03:
		do_bit_ops2( opc, opc2 );
		break;
	
case 0x90: case 0x91: case 0x92: case 0x93:
case 0x94: case 0x95: case 0x96: case 0x97:
		opcode( "callf" );
		operand( xref_genwordaddr( NULL, "$", MK_WORD(opc2, opc & 0x07) ) );
		break;

case 0x35: case 0x37: case 0x34: case 0x36:
		do_push_pop( opc, opc2 );
		break;
		
case 0x14:
		opcode( "br" );
		operand( xref_genwordaddr( NULL, "$", addr + (signed char)opc2 ) );
		break;
		
case 0x80: case 0x81: case 0x82: case 0x83:
case 0x84: case 0x85: case 0x86: case 0x87:
		do_bxx2( opc, opc2, addr );
		break;
		
case 0x32: case 0x33:
		opcode( "dbnz" );
		operand( "%s,%s", R2[opc & 0x01], xref_genwordaddr( NULL, "$", addr + (signed char)opc2 ) );
		break;
		
case 0x15:
		do_string_ops( opc, opc2 );
		break;
		
	
	}
	
	if ( output_buffer != outbuf )
	{
		return addr;
	}
	
	/* Three byte instructions 33333333333333333333333333333333333333333333333333 */
	opc3 = next( f, &addr );
	switch( opc )
	{
case 0x3A: case 0x2B:
		opcode( "mov" );
		operand( "%s,#" FORMAT_NUM_8BIT, 
			opc2 == 0xFE ? "PSWL" : 
				opc2 == 0xFF ? "PSWH" : 
				xref_genwordaddr( NULL, "$", opc2 + ( opc == 0x3A ? SADDR_OFFSET : SFR_OFFSET ) ), 
			opc3 );
		break;
		
case 0x38:
		opcode( "mov" );
		operand( "%s,%s", 
			xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), 
			xref_genwordaddr( NULL, "$", opc3 + SADDR_OFFSET ) );
		break;
		
case 0x39:
		opcode( "xch" );
		operand( "%s,%s", 
			xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), 
			xref_genwordaddr( NULL, "$", opc3 + SADDR_OFFSET ) );
		break;

case 0x60: case 0x61: case 0x62: case 0x63: 
case 0x64: case 0x65: case 0x66: case 0x67:
		opcode( "movw" );
		operand( "%s,#" FORMAT_NUM_16BIT, RP1[opc & 0x07],
			MK_WORD( opc2, opc3 ) );
		break;
		
case 0x3C:
		opcode( "movw" );
		operand( "%s,%s", 
			xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), 
			xref_genwordaddr( NULL, "$", opc3 + SADDR_OFFSET ) );
		break;
		
case 0x2A:
		opcode( "xchw" );
		operand( "%s,%s", 
			xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), 
			xref_genwordaddr( NULL, "$", opc3 + SADDR_OFFSET ) );
		break;

case 0x68: case 0x78:
		opcode( "add" );
		operand( "%s,", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ) );
		if ( opc & BIT(4) )
			operand( xref_genwordaddr( NULL, "$", opc3 + SADDR_OFFSET ) );
		else
			operand( "#" FORMAT_NUM_8BIT, opc3 );
		break;
	
case 0x69: case 0x79:
		opcode( "addc" );
		operand( "%s,", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ) );
		if ( opc & BIT(4) )
			operand( xref_genwordaddr( NULL, "$", opc3 + SADDR_OFFSET ) );
		else
			operand( "#" FORMAT_NUM_8BIT, opc3 );
		break;
		
case 0x6A: case 0x7A:
		opcode( "sub" );
		operand( "%s,", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ) );
		if ( opc & BIT(4) )
			operand( xref_genwordaddr( NULL, "$", opc3 + SADDR_OFFSET ) );
		else
			operand( "#" FORMAT_NUM_8BIT, opc3 );
		break;

case 0x6B: case 0x7B:
		opcode( "subc" );
		operand( "%s,", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ) );
		if ( opc & BIT(4) )
			operand( xref_genwordaddr( NULL, "$", opc3 + SADDR_OFFSET ) );
		else
			operand( "#" FORMAT_NUM_8BIT, opc3 );
		break;

case 0x6C: case 0x7C:
		opcode( "and" );
		operand( "%s,", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ) );
		if ( opc & BIT(4) )
			operand( xref_genwordaddr( NULL, "$", opc3 + SADDR_OFFSET ) );
		else
			operand( "#" FORMAT_NUM_8BIT, opc3 );
		break;		

case 0x6D: case 0x7D:
		opcode( "xor" );
		operand( "%s,", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ) );
		if ( opc & BIT(4) )
			operand( xref_genwordaddr( NULL, "$", opc3 + SADDR_OFFSET ) );
		else
			operand( "#" FORMAT_NUM_8BIT, opc3 );
		break;

case 0x6E: case 0x7E:
		opcode( "or" );
		operand( "%s,", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ) );
		if ( opc & BIT(4) )
			operand( xref_genwordaddr( NULL, "$", opc3 + SADDR_OFFSET ) );
		else
			operand( "#" FORMAT_NUM_8BIT, opc3 );
		break;

case 0x6F: case 0x7F:
		opcode( "cmp" );
		operand( "%s,", xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ) );
		if ( opc & BIT(4) )
			operand( xref_genwordaddr( NULL, "$", opc3 + SADDR_OFFSET ) );
		else
			operand( "#" FORMAT_NUM_8BIT, opc3 );
		break;

case 0x2D:
		opcode( "addw" );
		operand( "AX,#" FORMAT_NUM_16BIT, MK_WORD( opc2, opc3 ) );
		break;
		
case 0x3D:
		opcode( "addw" );
		operand( "%s,%s", 
			xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), 
			xref_genwordaddr( NULL, "$", opc3 + SADDR_OFFSET ) );
		break;
		
case 0x2E:
		opcode( "subw" );
		operand( "AX,#" FORMAT_NUM_16BIT, MK_WORD( opc2, opc3 ) );
		break;
		
case 0x3E:
		opcode( "subw" );
		operand( "%s,%s", 
			xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), 
			xref_genwordaddr( NULL, "$", opc3 + SADDR_OFFSET ) );
		break;

case 0x2F:
		opcode( "CMPw" );
		operand( "AX,#" FORMAT_NUM_16BIT, MK_WORD( opc2, opc3 ) );
		break;
		
case 0x3F:
		opcode( "cmpw" );
		operand( "%s,%s", 
			xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), 
			xref_genwordaddr( NULL, "$", opc3 + SADDR_OFFSET ) );
		break;

case 0x07:
		if ( opc2 == 0xEB )
		{
			opcode( "incw" );
			operand( xref_genwordaddr( NULL, "$", opc3 + SADDR_OFFSET ) );
		}
		else if ( opc2 == 0xE9 )
		{
			opcode( "decw" );
			operand( xref_genwordaddr( NULL, "$", opc3 + SADDR_OFFSET ) );
		}
		else
			do_07_3( opc2, opc3, addr );
		break;
		
case 0x28:
		opcode( "call" );
		operand( xref_genwordaddr( NULL, "$", MK_WORD( opc2, opc3 ) ) );
		break;

case 0x2C:
		opcode( "br" );
		operand( xref_genwordaddr( NULL, "$", MK_WORD( opc2, opc3 ) ) );
		break;
		
case 0x70: case 0x71: case 0x72: case 0x73:
case 0x74: case 0x75: case 0x76: case 0x77:
		opcode( "bt" );
		operand( "%s.%d,%s", 
			xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), 
			opc & 0x07, 
			xref_genwordaddr( NULL, "$", addr + (signed char)opc3 ) );
		break;
		
case 0x3B:
		opcode( "dbnz" );
		operand( "%s,%s", 
			xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), 
			xref_genwordaddr( NULL, "$", addr + (signed char)opc3 ) );
		break;
		
case 0x29:
		opcode( "retcs" );
		operand( xref_genwordaddr( NULL, "$", MK_WORD( opc2, opc3 ) ) );
		break;
		
case 0x01:
		do_01_3( opc2, opc3 );
		break;
		
case 0x08:
		do_08_3( opc2, opc3 );
		break;

case 0x02: case 0x03:
		do_bxx_3( opc, opc2, opc3, addr );
		break;		
	}
	
	if ( output_buffer != outbuf )
	{
		return addr;
	}
	
	/* 4-byte instructions 44444444444444444444444444444444444444444444444444444 */
	opc4 = next( f, &addr );
	switch( opc )
	{
case 0x06: case 0x0A: case 0x16: case 0x17:
		do_memmod_ops_4( opc, opc2, opc3, opc4 );
		break;
		
case 0x09:
		switch ( opc2 )
		{
	case 0xF0:
			opcode( "mov" );
			operand( "A,%s", xref_genwordaddr( NULL, "$", MK_WORD( opc3, opc4 ) ) );
			break;
			
	case 0xF1:
			opcode( "mov" );
			operand( "%s,A", xref_genwordaddr( NULL, "$", MK_WORD( opc3, opc4 ) ) );
			break;
			
	case 0x80: case 0x81: case 0x82: case 0x83:
	case 0x84: case 0x85: case 0x86: case 0x87:
			opcode( "movw" );
			operand( "%s,%s", RP1[opc2 & 0x07], xref_genwordaddr( NULL, "$", MK_WORD( opc3, opc4 ) ) );
			break;
			
	case 0x90: case 0x91: case 0x92: case 0x93:
	case 0x94: case 0x95: case 0x96: case 0x97:
			opcode( "movw" );
			operand( "%s,%s", xref_genwordaddr( NULL, "$", MK_WORD( opc3, opc4 ) ), RP1[opc2 & 0x07] );
			break;
			
	case 0x44:
			opcode( "mov" );
			operand( "stbc,#" FORMAT_NUM_8BIT, opc4 );
			break;
			
	case 0x42:
			opcode( "mov" );
			operand( "wdm,#" FORMAT_NUM_8BIT, opc4 );
			break;
		}
		break;
	
case 0x0C:
		opcode( "movw" );
		operand( "%s,#" FORMAT_NUM_16BIT, xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), MK_WORD( opc3, opc4 ) );
		break;
		
case 0x0B:
		opcode( "movw" );
		operand( "%s,#" FORMAT_NUM_16BIT, opc2 == 0xFC ? "SP" : xref_genwordaddr( NULL, "$", opc2 + SFR_OFFSET ), MK_WORD( opc3, opc4 ) );
		break;
		
case 0x01:
		switch( opc2 )
		{
	case 0x68:
			opcode( "add" );
			operand( "%s,#" FORMAT_NUM_8BIT, xref_genwordaddr( NULL, "$", opc3 + SFR_OFFSET ), opc4 );
			break;
	
	case 0x69:
			opcode( "addc" );
			operand( "%s,#" FORMAT_NUM_8BIT, xref_genwordaddr( NULL, "$", opc3 + SFR_OFFSET ), opc4 );
			break;
			
	case 0x6A:
			opcode( "sub" );
			operand( "%s,#" FORMAT_NUM_8BIT, xref_genwordaddr( NULL, "$", opc3 + SFR_OFFSET ), opc4 );
			break;
			
	case 0x6B:
			opcode( "subc" );
			operand( "%s,#" FORMAT_NUM_8BIT, xref_genwordaddr( NULL, "$", opc3 + SFR_OFFSET ), opc4 );
			break;
			
	case 0x6C:
			opcode( "and" );
			operand( "%s,#" FORMAT_NUM_8BIT, xref_genwordaddr( NULL, "$", opc3 + SFR_OFFSET ), opc4 );
			break;
			
	case 0x6E:
			opcode( "or" );
			operand( "%s,#" FORMAT_NUM_8BIT, xref_genwordaddr( NULL, "$", opc3 + SFR_OFFSET ), opc4 );
			break;
			
	case 0x6D:
			opcode( "xor" );
			operand( "%s,#" FORMAT_NUM_8BIT, xref_genwordaddr( NULL, "$", opc3 + SFR_OFFSET ), opc4 );
			break;
			
	case 0x6F:
			opcode( "cmp" );
			operand( "%s,#" FORMAT_NUM_8BIT, xref_genwordaddr( NULL, "$", opc3 + SFR_OFFSET ), opc4 );
			break;
		}
		break;
		
case 0x0D:
		opcode( "addw" );
		operand( "%s,#" FORMAT_NUM_16BIT, xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), MK_WORD( opc3, opc4 ) );
		break;
		
case 0x0E:
		opcode( "subw" );
		operand( "%s,#" FORMAT_NUM_16BIT, xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), MK_WORD( opc3, opc4 ) );
		break;
		
case 0x0F:
		opcode( "cmpw" );
		operand( "%s,#" FORMAT_NUM_16BIT, xref_genwordaddr( NULL, "$", opc2 + SADDR_OFFSET ), MK_WORD( opc3, opc4 ) );
		break;		
	
case 0x08:
		switch( opc2 & 0xF8 )
		{
	case 0xB8:
			opcode( "bt" );
			operand( "%s.%d," FORMAT_NUM_16BIT,
					xref_genwordaddr( NULL, "$", opc3 + SFR_OFFSET ),
					opc2 & 0x07,
					addr + (signed char)opc4 );
			break;
			
	case 0xA0:
			opcode( "bf" );
			operand( "%s.%d," FORMAT_NUM_16BIT,
					xref_genwordaddr( NULL, "$", opc3 + SADDR_OFFSET ),
					opc2 & 0x07,
					addr + (signed char)opc4 );
			break;
			
	case 0xA8:
			opcode( "bf" );
			operand( "%s.%d," FORMAT_NUM_16BIT,
					xref_genwordaddr( NULL, "$", opc3 + SFR_OFFSET ),
					opc2 & 0x07,
					addr + (signed char)opc4 );
			break;
			
	case 0xD0:
			opcode( "btclr" );
			operand( "%s.%d," FORMAT_NUM_16BIT,
					xref_genwordaddr( NULL, "$", opc3 + SADDR_OFFSET ),
					opc2 & 0x07,
					addr + (signed char)opc4 );
			break;
			
	case 0xD8:
			opcode( "btclr" );
			operand( "%s.%d," FORMAT_NUM_16BIT,
					xref_genwordaddr( NULL, "$", opc3 + SFR_OFFSET ),
					opc2 & 0x07,
					addr + (signed char)opc4 );
			break;
			
	case 0xC0:
			opcode( "bfset" );
			operand( "%s.%d," FORMAT_NUM_16BIT,
					xref_genwordaddr( NULL, "$", opc3 + SADDR_OFFSET ),
					opc2 & 0x07,
					addr + (signed char)opc4 );
			break;
			
	case 0xC8:
			opcode( "bfset" );
			operand( "%s.%d," FORMAT_NUM_16BIT,
					xref_genwordaddr( NULL, "$", opc3 + SFR_OFFSET ),
					opc2 & 0x07,
					addr + (signed char)opc4 );
			break;
		}
		break;
	}
	
	if ( output_buffer != outbuf )
	{
		return addr;
	}
	
	/* 5-byte instructions */
	opc5 = next( f, &addr );
	switch( opc )
	{
case 0x01:
		switch( opc2 )
		{
	case 0x0D:
			opcode( "addw" );
			operand( "%s,#" FORMAT_NUM_16BIT, xref_genwordaddr( NULL, "$", opc3 + SFR_OFFSET ), MK_WORD( opc4, opc5 ) );
			break;
			
	case 0x0E:
			opcode( "subw" );
			operand( "%s,#" FORMAT_NUM_16BIT, xref_genwordaddr( NULL, "$", opc3 + SFR_OFFSET ), MK_WORD( opc4, opc5 ) );
			break;

	case 0x0F:
			opcode( "cmpw" );
			operand( "%s,#" FORMAT_NUM_16BIT, xref_genwordaddr( NULL, "$", opc2 + SFR_OFFSET ), MK_WORD( opc3, opc4 ) );
			break;
		}
		break;
	}
	
	if ( output_buffer != outbuf )
	{
		return addr;
	}

	/* Unknown */	
	opcode( "???" );
	operand( "........%02X %02X %02X", opc, opc2, opc3 );
   return addr;
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
