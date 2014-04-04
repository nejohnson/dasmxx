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


/******************************************************************************/
/* TODO */
/*

	- add xrefs
	- try out table-driven parser
	- find bugs and fix'em
	




*/


//#define USE_RECURSIVE_DESCENT_PARSER
#define USE_TABLE_PARSER


const char * dasm_name            = "dasm78k3";
const char * dasm_description     = "NEC 78K/III";
const int    dasm_max_insn_length = 5;





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


#define INSN_FOUND			( 1 )
#define INSN_NOT_FOUND	( 0 )


static char * output_buffer = NULL;



/*****************************************************************************
 *        Private Functions
 *****************************************************************************/



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



/*********************************************************************************/
#if defined(USE_RECURSIVE_DESCENT_PARSER)
/*********************************************************************************/

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
		
case 0x19:
		opcode( arg == 0x00 ? "incw" : "decw" );
		operand( "SP" );
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
 
ADDR dasm_insn( FILE *f, char *outbuf, ADDR addr )
{
	UBYTE opc, opc2, opc3, opc4, opc5;
	
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
#elif defined(USE_TABLE_PARSER)
/******************************************************************************/



/**
	Neaten up emitting a comma ","
**/
#define COMMA	operand( ", " )

/******************************************************************************/
/**                            Empty Operands                                **/
/******************************************************************************/

static void operand_none( FILE * f, ADDR * addr, UBYTE opc )
{
	/* empty */
}

/******************************************************************************/
/**                            Single Operands                               **/
/******************************************************************************/

/***********************************************************
 *
 * FUNCTION
 *      operand_r
 *
 * DESCRIPTION
 *      Process "r" operand.
 *			r comes from opc byte.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_r( FILE * f, ADDR * addr, UBYTE opc )
{
	UBYTE r = opc & 0x0F;
	
	operand( "R%d", r );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_r1
 *
 * DESCRIPTION
 *      Process "r1" operand.
 *			r1 comes from opc byte.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_r1( FILE * f, ADDR * addr, UBYTE opc )
{
	UBYTE r1 = opc & 0x07;
	
	operand( "R%d", r1 );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_rp
 *
 * DESCRIPTION
 *      Process "r" operand.
 *			r comes from opc byte.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_rp( FILE * f, ADDR * addr, UBYTE opc )
{
	UBYTE rp = opc & 0x07;
	
	operand( "%s", RP[rp] );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_rp1
 *
 * DESCRIPTION
 *      Process "rp1" operand.
 *			rp1 comes from opc byte.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_rp1( FILE * f, ADDR * addr, UBYTE opc )
{
	UBYTE rp1 = opc & 0x07;
	
	operand( "%s", RP1[rp1] );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_rp2
 *
 * DESCRIPTION
 *      Process "rp2" operands.
 *			r1 comes from opc byte.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_rp2( FILE * f, ADDR * addr, UBYTE opc )
{
	UBYTE rp2 = opc & 0x03;
	
	operand( "%s", RP2[rp2] );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_byte
 *
 * DESCRIPTION
 *      Process "#byte" operands.
 *			byte comes from opc byte.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_byte( FILE * f, ADDR * addr, UBYTE opc )
{
	operand( "#" FORMAT_NUM_8BIT, opc );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_saddr
 *
 * DESCRIPTION
 *      Process "saddr" operands.
 *			saddr comes from opc byte.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_saddr( FILE * f, ADDR * addr, UBYTE opc )
{
	operand( FORMAT_NUM_16BIT, opc + SADDR_OFFSET );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_sfr
 *
 * DESCRIPTION
 *      Process "sfr" operands.
 *			sfr comes from opc byte.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_sfr( FILE * f, ADDR * addr, UBYTE opc )
{
	if ( opc == 0xFE )
		operand( "PSWL" );
	else if ( opc == 0xFF )
		operand( "PSWH" );
	else
		operand( FORMAT_NUM_16BIT, opc + SFR_OFFSET );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_mem
 *
 * DESCRIPTION
 *      Process "mem" operand, which is a restricted subset
 *			of the register-indirect mode.
 *			mem comes from opc byte.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_mem( FILE * f, ADDR * addr, UBYTE opc )
{
	UBYTE mem = opc & 0x07;
	
	operand( "%s", MEM_MOD_RI[mem] );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_memmod
 *
 * DESCRIPTION
 *      Process larger "mem" operand.
 *			quite a tricky jobby.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_memmod( FILE * f, ADDR * addr, UBYTE opc )
{
	UBYTE mod = opc & 0x1F;
	UBYTE mem, low_offset, high_offset;
	
	mem = next( f, addr );
	mem = ( mem >> 4 ) & 0x07;

	low_offset  = next( f, addr );
	high_offset = next( f, addr );
	
	if ( mod == 0x16 )
		operand_mem( f, addr, mem );
	else if ( mod == 0x17 )
		operand( "%s", MEM_MOD_BI[mem] );
	else if ( mod == 0x06 )
		operand( "%s" FORMAT_NUM_8BIT "]", MEM_MOD_BASE[mem], low_offset );
	else if ( mod == 0x0A )
		operand( FORMAT_NUM_16BIT "%s", MK_WORD(low_offset, high_offset), MEM_MOD_INDEX[mem] );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_addr16
 *
 * DESCRIPTION
 *      Process "addr16" operand.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_addr16( FILE * f, ADDR * addr, UBYTE opc )
{
	UBYTE low_addr, high_addr;

	low_addr  = next( f, addr );
	high_addr = next( f, addr );
	
	operand( FORMAT_NUM_16BIT, MK_WORD( low_addr, high_addr ) );
}

/******************************************************************************/
/**                           Double Operands                                **/
/******************************************************************************/

/***********************************************************
 *
 * FUNCTION
 *      operand_r1_byte
 *
 * DESCRIPTION
 *      Process "r1,#byte" operands.
 *			r1 comes from opc byte, byte comes from next byte.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_r1_byte( FILE * f, ADDR * addr, UBYTE opc )
{
	UBYTE data = next( f, addr );
	
	operand_r1( f, addr, opc );
	COMMA;
	operand_byte( f, addr, data );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_saddr_byte
 *
 * DESCRIPTION
 *      Process "saddr,#byte" operands.
 *			saddr comes from next byte, byte comes from next byte.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_saddr_byte( FILE * f, ADDR * addr, UBYTE opc )
{
	UBYTE saddr_offset = next( f, addr );
	UBYTE data         = next( f, addr );
	
	operand_saddr( f, addr, saddr_offset );
	COMMA;
	operand_byte( f, addr, data );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_sfr_byte
 *
 * DESCRIPTION
 *      Process "sfr,#byte" operands.
 *			sfr comes from next byte, byte comes from next byte.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_sfr_byte( FILE * f, ADDR * addr, UBYTE opc )
{
	UBYTE sfr_offset = next( f, addr );
	UBYTE data       = next( f, addr );
	
	operand_sfr( f, addr, sfr_offset );
	COMMA;
	operand_byte( f, addr, data );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_r_r1
 *
 * DESCRIPTION
 *      Process "r,r1" operands.
 *			both registers come from next byte.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_r_r1( FILE * f, ADDR * addr, UBYTE opc )
{
	UBYTE regs = next( f, addr );
	
	operand_r( f, addr, regs >> 4 );
	COMMA;
	operand_r1( f, addr, regs );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_rp_rp1
 *
 * DESCRIPTION
 *      Process "rp,rp1" operands.
 *			both registers come from next byte.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_rp_rp1( FILE * f, ADDR * addr, UBYTE opc )
{
	UBYTE regs = next( f, addr );
	
	operand_rp( f, addr, regs >> 5 );
	COMMA;
	operand_rp1( f, addr, regs );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_A_r1
 *
 * DESCRIPTION
 *      Process "A,r1" operands.
 *			r1 come from opc byte.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_A_r1( FILE * f, ADDR * addr, UBYTE opc )
{
	operand( "A" );
	COMMA;
	operand_r1( f, addr, opc );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_A_saddr
 *
 * DESCRIPTION
 *      Process "A,saddr" operands.
 *			saddr come from next byte.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_A_saddr( FILE * f, ADDR * addr, UBYTE opc )
{
	operand( "A" );
	COMMA;
	operand_saddr( f, addr, next( f, addr ) );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_saddr_A
 *
 * DESCRIPTION
 *      Process "saddr,A" operands.
 *			saddr come from next byte.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_saddr_A( FILE * f, ADDR * addr, UBYTE opc )
{
	operand_saddr( f, addr, next( f, addr ) );
	COMMA;
	operand( "A" );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_saddr_saddr
 *
 * DESCRIPTION
 *      Process "saddr,saddr" operands.
 *			saddrs come from next bytes.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_saddr_saddr( FILE * f, ADDR * addr, UBYTE opc )
{
	operand_saddr( f, addr, next( f, addr ) );
	COMMA;
	operand_saddr( f, addr, next( f, addr ) );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_A_sfr
 *
 * DESCRIPTION
 *      Process "A,sfr" operands.
 *			sfr comes from next byte.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_A_sfr( FILE * f, ADDR * addr, UBYTE opc )
{
	operand( "A" );
	COMMA;
	operand_sfr( f, addr, next( f, addr ) );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_sfr_A
 *
 * DESCRIPTION
 *      Process "sfr,A" operands.
 *			sfr comes from next byte.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_sfr_A( FILE * f, ADDR * addr, UBYTE opc )
{
	operand_sfr( f, addr, next( f, addr ) );
	COMMA;
	operand( "A" );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_A_mem
 *
 * DESCRIPTION
 *      Process "A,mem" operands.
 *			mem comes from opc byte translated.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_A_mem( FILE * f, ADDR * addr, UBYTE opc )
{
	operand( "A" );
	COMMA;
	operand_mem( f, addr, opc );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_mem_A
 *
 * DESCRIPTION
 *      Process "mem,A" operands.
 *			mem comes from opc byte translated.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_mem_A( FILE * f, ADDR * addr, UBYTE opc )
{
	operand_mem( f, addr, opc );
	COMMA;
	operand( "A" );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_A_memmod
 *
 * DESCRIPTION
 *      Process larger "A,mem" operands.
 *			quite a tricky jobby.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_A_memmod( FILE * f, ADDR * addr, UBYTE opc )
{
	operand( "A" );
	COMMA;
	operand_memmod( f, addr, opc );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_memmod_A
 *
 * DESCRIPTION
 *      Process larger "mem,A" operands.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_memmod_A( FILE * f, ADDR * addr, UBYTE opc )
{
	operand_memmod( f, addr, opc );
	COMMA;
	operand( "A" );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_A_saddrp
 *
 * DESCRIPTION
 *      Process "A,[saddrp]" operands.
 *			saddr come from next byte.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_A_saddrp( FILE * f, ADDR * addr, UBYTE opc )
{
	operand( "A" );
	COMMA;
	operand( "[" );
	operand_saddr( f, addr, next( f, addr ) );
	operand( "]" );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_saddrp_A
 *
 * DESCRIPTION
 *      Process "[saddrp],A" operands.
 *			saddr come from next byte.
 *
 * RETURNS
 *      void
 *
 ************************************************************/
 
static void operand_saddrp_A( FILE * f, ADDR * addr, UBYTE opc )
{
	operand( "[" );
	operand_saddr( f, addr, next( f, addr ) );
	operand( "]" );
	COMMA;
	operand( "A" );
}

/***********************************************************
 *
 * FUNCTION
 *      operand_A_addr16
 *
 * DESCRIPTION
 *      Process "A,!addr16" operands.
 *			addr16 come from next bytes.
 *
 * RETURNS
 *      void
 *
 ************************************************************/

static void operand_A_addr16( FILE * f, ADDR * addr, UBYTE opc )
{
	operand( "A" );
	COMMA;
	operand( "!" );
	operand_addr16( f, addr, opc );
}
 
/***********************************************************
 *
 * FUNCTION
 *      operand_addr16_A
 *
 * DESCRIPTION
 *      Process "!addr16,A" operands.
 *			addr16 come from next bytes.
 *
 * RETURNS
 *      void
 *
 ************************************************************/

static void operand_addr16_A( FILE * f, ADDR * addr, UBYTE opc )
{
	operand( "!" );
	operand_addr16( f, addr, opc );
	COMMA;
	operand( "A" );
}








/******************************************************************************/

/**
	The optab_t type describes each entry in the op tables.
**/
typedef struct optab_s {
	UBYTE opc;
	const char * opcode;
	void (*operands)( FILE *, ADDR *, UBYTE); /* operand function */
	enum {
		OPTAB_INSN,
		OPTAB_RANGE,
		OPTAB_MASK,
		OPTAB_MASK2,
		OPTAB_MEMMOD,
		OPTAB_TABLE
	} type;
	union {
		struct {
			UBYTE min, max;
		} range;
		struct {
			UBYTE mask, val;
		} mask;
		struct optab_s * table;
	} u;
} optab_t;

/******************************************************************************/

/**
	Macros to construct entries in op tables.
**/

/**
	The given instruction byte jumps to another table.
**/
#define TABLE(M_tablename, M_opc)					{ 	.type    = OPTAB_TABLE,			\
																.opc     = M_opc, 					\
																.opcode  = "TABLE",				\
																.u.table = M_tablename 			\
															},

/**
	A single insruction matches against one op byte.
**/	
#define INSN(M_opcode, M_ops, M_opc)				{ 	.type     = OPTAB_INSN, 			\
																.opc      = M_opc, 				\
																.opcode   = M_opcode, 			\
																.operands = operand_ ## M_ops 	\
															},

/**
	A RANGE matches the first byte anywhere between M_min and M_max inclusive.
**/	
#define RANGE(M_opcode, M_ops, M_min, M_max)	{ 	.type     = OPTAB_RANGE, 		\
																.opcode   = M_opcode,				\
																.operands = operand_ ## M_ops, \
																.u.range.min = M_min,				\
																.u.range.max = M_max				\
															},

/**
	A MASK matches a set of instruction bytes described by a bit mask and a
   value to match against applied to the first search byte.
**/	
#define MASK(M_opcode, M_ops, M_mask, M_val)  {	.type     = OPTAB_MASK,			\
																.opcode   = M_opcode,				\
																.operands = operand_ ## M_ops, \
																.u.mask.mask = M_mask,			\
																.u.mask.val  = M_val				\
															},

/**
	A MASK2 matches a set of instruction bytes described by a bit mask and a
   value to match against applied to the second search byte.
**/	
#define MASK2(M_opcode, M_ops, M_opc, M_mask, M_val) 									\
															{	.type     = OPTAB_MASK2,			\
																.opcode   = M_opcode,				\
																.opc      = M_opc, 				\
																.operands = operand_ ## M_ops, \
																.u.mask.mask = M_mask,			\
																.u.mask.val  = M_val				\
															},
															
/**
	A MASK2 matches a set of instruction bytes described by a bit mask and a
   value to match against applied to the second search byte.
**/	
#define MEMMOD(M_opcode, M_ops, M_opc) 			{	.type     = OPTAB_MEMMOD,		\
																.opcode   = M_opcode,				\
																.opc      = M_opc, 				\
																.operands = operand_ ## M_ops	\
															},
															
/**
	Mark end of op table.
**/
#define END		{ .opcode = NULL }

/******************************************************************************/

static optab_t optab_01[] = {
	
	INSN  ( "xch",  A_sfr,    0x21 )


	END
};

/******************************************************************************/

static optab_t optab_09[] = {
	
	INSN  ( "mov",  A_addr16, 0xF0 )
	INSN  ( "mov",  addr16_A, 0xF1 )


	END
};

/******************************************************************************/

static optab_t base_optab[] = {

/*----------------------------------------------------------------------------
  Data Transfer
  ----------------------------------------------------------------------------*/

	RANGE ( "mov",  r1_byte,     0xB8, 0xBF )
	INSN  ( "mov",  saddr_byte,  0x3A )
	INSN  ( "mov",  sfr_byte,    0x2B )
	MASK2 ( "mov",  r_r1,        0x22, 0x08, 0x00 )
	RANGE ( "mov",  A_r1,        0xD0, 0xD7 )
	INSN  ( "mov",  A_saddr,     0x20 )
	INSN  ( "mov",  saddr_A,     0x22 )
	INSN  ( "mov",  saddr_saddr, 0x38 )
	INSN  ( "mov",  A_sfr,       0x10 )
	INSN  ( "mov",  sfr_A,       0x12 )
	RANGE ( "mov",  A_mem,       0x58, 0x5D )
	MEMMOD( "mov",  A_memmod,    0x00 ) 
	RANGE ( "mov",  mem_A,       0x50, 0x55 )
	MEMMOD( "mov",  memmod_A,    0x80 )
	INSN  ( "mov",  A_saddrp,    0x18 )
	INSN  ( "mov",  saddrp_A,    0x19 )
	
	
	RANGE ( "xch",  A_r1,        0xD8, 0xDF )
	MASK2 ( "xch",  r_r1,        0x25, 0x08, 0x00 )
	MEMMOD( "xch",  A_memmod,    0x04 )
	INSN  ( "xch",  A_saddr,     0x21 )
	INSN  ( "xch",  A_saddrp,    0x23 )
	INSN  ( "mov",  saddr_saddr, 0x39 )
	
	
	
	
	
	MASK2 ( "movw", rp_rp1,      0x22, 0x08, 0x08 )
	
	
	
	
	
	
	


	RANGE( "incw", rp2, 0x44, 0x47 )
	
	RANGE( "decw", rp2, 0x4C, 0x4F )

	INSN( "ei", none, 0x4B )

	
	
	
	RANGE( "inc",  r1,         0xC0, 0xC7 )
	RANGE( "dec",  r1,         0xC8, 0xCF )
	
	
	
	
	
	TABLE ( optab_01, 0x01 )
	TABLE ( optab_09, 0x09 )

	
	
	END
};

/***********************************************************
 *
 * FUNCTION
 *      walk_table
 *
 * DESCRIPTION
 *      Disassembles the next instruction in the input stream.
 *      f - file stream to read (pass to calls to next() )
 *      outbuf - pointer to output buffer
 *      addr - address of first input byte for this insn
 *
 * RETURNS
 *      1 if a valid instruction found, else 0.
 *
 ************************************************************/

static int walk_table( FILE * f, ADDR * addr, optab_t * optab, UBYTE opc )
{
	UBYTE peek_byte;
	int have_peeked = 0;
	
	if ( optab == NULL )
		return;
		
	while ( optab->opcode != NULL )
	{
	   /* printf("type:%d  ", optab->type); */
		if ( optab->type == OPTAB_TABLE && optab->opc == opc )
		{
			opc = next( f, addr );
			return walk_table( f, addr, optab->u.table, opc );
		}
		else if ( ( optab->type == OPTAB_INSN && opc == optab->opc )
					||
					 ( optab->type == OPTAB_RANGE && opc >= optab->u.range.min && opc <= optab->u.range.max )
					||
					 ( optab->type == OPTAB_MASK && ( ( opc & optab->u.mask.mask ) == optab->u.mask.val ) ) )
		{
			opcode( optab->opcode );
			optab->operands( f, addr, opc );
			return INSN_FOUND;
		}
		else if ( optab->type == OPTAB_MASK2 && opc == optab->opc )
		{
			if ( !have_peeked )
			{
				peek_byte = peek( f );
				have_peeked = 1;
			}
			
			if ( ( peek_byte & optab->u.mask.mask ) == optab->u.mask.val )
			{
				opcode( optab->opcode );
				optab->operands( f, addr, opc );
				return INSN_FOUND;
			}
		}
		else if ( optab->type == OPTAB_MEMMOD 
					&& ( opc == 0x16 || opc == 0x17 || opc == 0x06 || opc == 0x0A ) )
		{
			if ( !have_peeked )
			{
				peek_byte = peek( f );
				have_peeked = 1;
			}
			
			if ( ( peek_byte & 0x8F ) == optab->opc )
			{
				opcode( optab->opcode );
				optab->operands( f, addr, opc );
				return INSN_FOUND;
			}		
		}
		
		optab++;	
	}
	
	return INSN_NOT_FOUND;
}

/***********************************************************
 *
 * FUNCTION
 *      dasm_insn
 *
 * DESCRIPTION
 *      Disassembles the next instruction in the input stream.
 *      f - file stream to read (pass to calls to next() )
 *      outbuf - pointer to output buffer
 *      addr - address of first input byte for this insn
 *
 * RETURNS
 *      address of next input byte
 *
 ************************************************************/
 
ADDR dasm_insn( FILE *f, char *outbuf, ADDR addr )
{
	int opc;
	UWORD insn_addr = addr;
	int found = 0;
	
	/* Setup output_buffer to point to caller's output buffer */
	output_buffer = outbuf;

	/* Get first opcode byte */
	opc = next( f, &addr );

	/* Now walk table(e) looking for an instruction */
	found = walk_table( f, &addr, base_optab, opc );
	
	/* If we didn't find a match, indicate this to the output */
	if ( found != INSN_FOUND )
		opcode( "???" );
	
	return addr;
}

/******************************************************************************/
#else
/******************************************************************************/

#error Undefined parser type!!!

/******************************************************************************/
#endif
/******************************************************************************/


/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
