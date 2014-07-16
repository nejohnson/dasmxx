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
 
/*****************************************************************************
 * Note: the AVR instruction encoding is based on 16-bit words, with most
 *       instructions being one word, and a few (e.g., JMP) being two (i.e.,
 *       32 bits long).  Because of this the base instruction unit is set to
 *       UWORD.
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "dasmxx.h"

/*****************************************************************************
 * Globally-visible decoder properties
 *****************************************************************************/

/* Decoder short name */
const char * dasm_name            = "dasmavr";

/* Decoder description */
const char * dasm_description     = "Atmel AVR";

/* Decoder maximum instruction length in bytes */
const int    dasm_max_insn_length = 4;

/* Decoder maximum opcode field width */
const int    dasm_max_opcode_width = 9;

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/
 
typedef UWORD OPC;

/* Common output formats */
#define FORMAT_NUM_8BIT		"$%02X"
#define FORMAT_NUM_16BIT		"$%04X"
#define FORMAT_REG				"R%d"

/* Create a single-bit mask */
#define BIT(n)					( 1 << (n) )

/* Construct a 16-bit word out of low and high bytes */
#define MK_WORD(l,h)			( ((l) & 0xFF) | (((UWORD)((h) & 0xFF)) << 8) )

/* Indicate whether decode was successful or not. */
#define INSN_FOUND				( 1 )
#define INSN_NOT_FOUND		( 0 )

/* Neaten up emitting a comma "," within an operand. */
#define COMMA	operand( ", " )

/**
	The optab_t type describes each entry in the op tables.
**/
typedef struct optab_s {
	OPC opc;
	const char * opcode;
	void (*operands)( FILE *, ADDR *, OPC, XREF_TYPE); /* operand function */
	XREF_TYPE xtype;
	enum {
		OPTAB_INSN,
		OPTAB_RANGE,
		OPTAB_MASK,
		OPTAB_MASK2,
		OPTAB_TABLE
	} type;
	union {
		struct {
			OPC min, max;
		} range;
		struct {
			OPC mask, val;
		} mask;
		struct optab_s * table;
	} u;
} optab_t;

/**
	Macros to construct entries in op tables.
**/

/**
	The given instruction byte jumps to another decode table.
**/
#define TABLE(M_tablename, M_opc)					{ 	.type    = OPTAB_TABLE,			\
																.opc     = M_opc, 					\
																.opcode  = "TABLE",				\
																.u.table = M_tablename 			\
															},

/**
	A single insruction matches against one op byte.
**/	
#define INSN(M_opcode, M_ops, M_opc, M_xt)		{ 	.type     = OPTAB_INSN, 			\
																.opc      = M_opc, 				\
																.opcode   = M_opcode, 			\
																.operands = operand_ ## M_ops,	\
																.xtype    = M_xt					\
															},

/**
	A RANGE matches the first byte anywhere between M_min and M_max inclusive.
**/	
#define RANGE(M_opcode, M_ops, M_min, M_max, M_xt) 	\
															{ 	.type     = OPTAB_RANGE, 		\
																.opcode   = M_opcode,				\
																.operands = operand_ ## M_ops, \
																.xtype    = M_xt,					\
																.u.range.min = M_min,				\
																.u.range.max = M_max				\
															},

/**
	A MASK matches a set of instruction bytes described by a bit mask and a
   value to match against applied to the first search byte.
**/	
#define MASK(M_opcode, M_ops, M_mask, M_val, M_xt)  	\
															{	.type     = OPTAB_MASK,			\
																.opcode   = M_opcode,				\
																.operands = operand_ ## M_ops, \
																.xtype    = M_xt,					\
																.u.mask.mask = M_mask,			\
																.u.mask.val  = M_val				\
															},

/**
	Mark end of op table.
**/
#define END		{ .opcode = NULL }

/*****************************************************************************
 * Private data.
 *****************************************************************************/

/* Start address of each instruction as it is decoded. */
static ADDR g_insn_addr = 0;

/* Global output buffer into which the decoded output is written. */
static char * g_output_buffer = NULL;

/*****************************************************************************
 *        Private Functions
 *****************************************************************************/

/***********************************************************
 *
 * FUNCTION
 *      opcode
 *
 * DESCRIPTION
 *      Writes the given opcode string into the output buffer.
 *
 * RETURNS
 *      none
 *
 ************************************************************/
 
static void opcode( const char *opcode )
{
	int n = sprintf( g_output_buffer, "%-*s", dasm_max_opcode_width, opcode );
	g_output_buffer += n;
}

/***********************************************************
 *
 * FUNCTION
 *      operand
 *
 * DESCRIPTION
 *      Writes the given operand string and any arguments
 *      into the output buffer.  The string is processed with
 *      the usual printf() conversions.
 *
 * RETURNS
 *      none
 *
 ************************************************************/
 
static void operand( const char *operand, ... )
{
	va_list ap;
	int n;
	
	va_start( ap, operand );
	n = vsprintf( g_output_buffer, operand, ap );
	va_end( ap );
	
	g_output_buffer += n;
}

/******************************************************************************/
/**                            Operand Functions                             **/
/******************************************************************************/

#define OPERAND_FUNC(M_name) \
	static void operand_ ## M_name (FILE *f, ADDR * addr, OPC opc, XREF_TYPE xtype )

/******************************************************************************/
/**                            Empty Operands                                **/
/******************************************************************************/

OPERAND_FUNC(none)
{
	/* empty */
}

/******************************************************************************/
/**                            Single Operands                               **/
/******************************************************************************/

/***********************************************************
 * Full-range (5-bit) destination register addressing.
 *	Register number is encoded in the opcode:
 *  15       8 7       0
 *   ---- ---d dddd ----
 ************************************************************/
OPERAND_FUNC(rD5)
{
	int Rd = ( opc >> 4 ) & 0x1F;
	
	operand( FORMAT_REG, Rd );
}

/***********************************************************
 * 7-bit signed branch offset
 *	Register number is encoded in the opcode:
 *  15       8 7       0
 *   ---- --kk kkkk k---
 * Note that the offset is a signed number, in the range
 * -64 <= k <= 63.
 ************************************************************/
OPERAND_FUNC(bra7)
{
	BYTE disp = ((BYTE)(opc >> 2 )) / 2; /* SIGNED arithmetic! */
	ADDR dest = *addr + ( 2 * disp );
	
	operand( xref_genwordaddr( NULL, "$", dest ) );
	xref_addxref( xtype, g_insn_addr, dest );
}

/***********************************************************
 * 12-bit signed offset
 *  15       8 7       0
 *   ---- kkkk kkkk kkkk
 * Note that the offset is a signed number, in the range
 * -2k <= k <= 2k.
 ************************************************************/
OPERAND_FUNC(rel_k12)
{
	WORD k = (WORD)(( opc << 4) & 0xFFF0 ) / 16;
	
	ADDR dest = *addr + ( k * 2 );
	
	operand( xref_genwordaddr( NULL, "$", dest ) );
	xref_addxref( xtype, g_insn_addr, dest ); 
}

/***********************************************************
 * 22-bit long address, encoded in two opcs.
 *  15       8 7       0    15                 0
 *   ---- ---k kkkk ---k     kkkk kkkk kkkk kkkk
 ************************************************************/
OPERAND_FUNC(long_addr)
{
	ADDR dest = (ADDR)nextw( f, addr );
	dest |= ( opc & 0x0001 ) << 16;
	dest |= ( opc & 0x01F0 ) << 13;
	
	operand( xref_genwordaddr( NULL, "$", dest ) );
	xref_addxref( xtype, g_insn_addr, dest ); 
}

/***********************************************************
 * Bit index.
 *  15       8 7       0
 *   ---- ---- -sss ----
 ************************************************************/
OPERAND_FUNC(bit)
{
	int s = ( opc >> 4 ) & 0x07;
	
	operand( "%d", s );
}

/******************************************************************************/
/**                            Double Operands                               **/
/******************************************************************************/

/***********************************************************
 * Two register pairs
 *	Register numbers are encoded in the opcode:
 *  15       8 7       0
 *   ---- ---- dddd rrrr
 ************************************************************/
OPERAND_FUNC(rpair_rpair)
{
	int Rd = ( ( opc >> 4 ) & 0x0F ) * 2;
	int Rr = ( opc & 0x0F ) * 2;
	
	operand( FORMAT_REG ":" FORMAT_REG, Rd + 1, Rd );
	COMMA;
	operand( FORMAT_REG ":" FORMAT_REG, Rr + 1, Rr );
}

/***********************************************************
 * Two registers, both high set (16..31)
 *	Register numbers are encoded in the opcode:
 *  15       8 7       0
 *   ---- ---- dddd rrrr
 ************************************************************/
OPERAND_FUNC(rhigh_rhigh)
{
	int Rd = ( ( opc >> 4 ) & 0x0F ) + 16;
	int Rr = ( opc & 0x0F ) + 16;
	
	operand( FORMAT_REG, Rd );
	COMMA;
	operand( FORMAT_REG, Rr );
}

/***********************************************************
 * Two registers, both limited high set (16..23)
 *	Register numbers are encoded in the opcode:
 *  15       8 7       0
 *   ---- ---- -ddd -rrr
 ************************************************************/
OPERAND_FUNC(rhigh3_rhigh3)
{
	int Rd = ( ( opc >> 4 ) & 0x07 ) + 16;
	int Rr = ( opc & 0x07 ) + 16;
	
	operand( FORMAT_REG, Rd );
	COMMA;
	operand( FORMAT_REG, Rr );
}

/***********************************************************
 * General reg-reg addressing.
 *	Register numbers are encoded in the opcode:
 *  15       8 7       0
 *   ---- --rd dddd rrrr
 ************************************************************/
OPERAND_FUNC(r_r)
{
	int Rr = ( ( opc >> 5 ) & 0x10 ) | ( opc & 0x0F );
	
	operand_rD5( f, addr, opc, xtype );
	COMMA;
	operand( FORMAT_REG, Rr );
}

/***********************************************************
 * Single high register and 8-bit constant.
 *	Encoding:
 *  15       8 7       0
 *   ---- kkkk dddd kkkk
 ************************************************************/
OPERAND_FUNC(rhigh_k8)
{
	int Rd = ( ( opc >> 4 ) & 0x0F ) + 16;
	int K  = ( ( opc >> 4 ) & 0xF0 ) | ( opc & 0x0F );

	operand( FORMAT_REG, Rd );
	COMMA;
	operand( FORMAT_NUM_8BIT, K );
}

/***********************************************************
 * Single register load from address in Y or Z register with
 * optional offset Q.
 *	Encoding:
 *  15       8 7       0
 *   --q- qq-d dddd Aqqq
 ************************************************************/
OPERAND_FUNC(r_YZ)
{
	int A = opc & 0x0008;
	int Q = ( opc & 0x07 ) | ( ( opc >> 8 ) & 0x18 ) | ( ( opc >> 8 ) & 0x20 );
	
	operand_rD5( f, addr, opc, xtype );
	COMMA;
	operand( A ? "Y" : "Z" );
	if ( Q )
		operand( "+%d", Q );
}

/***********************************************************
 * Single register store from address in Y or Z register with
 * optional offset Q.
 *	Encoding:
 *  15       8 7       0
 *   --q- qq-d dddd Aqqq
 ************************************************************/
OPERAND_FUNC(YZ_r)
{
	int A = opc & 0x0008;
	int Q = ( opc & 0x07 ) | ( ( opc >> 8 ) & 0x18 ) | ( ( opc >> 8 ) & 0x20 );
	
	operand( A ? "Y" : "Z" );
	if ( Q )
		operand( "+%d", Q );
	COMMA;
	operand_rD5( f, addr, opc, xtype );
}

/***********************************************************
 * Single full-range register and bit index
 *	Encoding:
 *  15       8 7       0
 *   ---- ---d dddd -bbb
 ************************************************************/
OPERAND_FUNC(r_b)
{
	int b = opc & 0x07;
	
	operand_rD5( f, addr, opc, xtype );
	COMMA;
	operand( "%d", b );
}
	
/***********************************************************
 * I/O register and bit
 *	Encoding:
 *  15       8 7       0
 *   ---- ---- AAAA Abbb
 ************************************************************/
OPERAND_FUNC(A_b)
{
	int A = ( opc >> 3 ) & 0x1F;
	int b = opc & 0x07;
	
	operand( FORMAT_NUM_8BIT, A );
	COMMA;
	operand( "%d", b );
}

/***********************************************************
 * Highest register pairs, plus 6-bit K
 *	Encoding:
 *  15       8 7       0
 *   ---- ---- kkRR kkkk
 ************************************************************/
OPERAND_FUNC(rphigh_k6)
{
	int R = ( ( opc >> 4 ) & 0x03 );
	int k = ( opc & 0x0F ) | ( ( opc >> 2 ) & 0x30 );
	static char *rpair[] = {
		"R25:R24",
		"XH:XL",
		"YH:YL",
		"ZH:ZL"
	};
	
	operand( "%s", rpair[R] );
	COMMA;
	operand( "%d", k );
}

/***********************************************************
 * Load full register from I/O location
 *	Encoding:
 *  15       8 7       0
 *   ---- -AAr rrrr AAAA
 ************************************************************/
OPERAND_FUNC(r_A6)
{
	UBYTE A = ( opc & 0x0F ) | ( ( opc >> 5 ) & 0x30 );
	
	operand_rD5( f, addr, opc, xtype );
	COMMA;
	operand( xref_genwordaddr( NULL, "$", A ) );
	xref_addxref( xtype, g_insn_addr, A );
}

/***********************************************************
 * Store full register to I/O location
 *	Encoding:
 *  15       8 7       0
 *   ---- -AAr rrrr AAAA
 ************************************************************/
OPERAND_FUNC(A6_r)
{
	UBYTE A = ( opc & 0x0F ) | ( ( opc >> 5 ) & 0x30 );
	
	operand( xref_genwordaddr( NULL, "$", A ) );
	xref_addxref( xtype, g_insn_addr, A );
	COMMA;
	operand_rD5( f, addr, opc, xtype );
}

/***********************************************************
 * Load full register from Z
 *	Encoding:
 *  15       8 7       0
 *   ---- ---r rrrr --mm
 ************************************************************/
OPERAND_FUNC(r_Zpm)
{
	enum {
		STATIC  = 0x00,
		POSTINC = 0x01,
		PREDEC  = 0x02
	} mode = opc & 0x03;
	
	operand_rD5( f, addr, opc, xtype );
	COMMA;
	switch ( mode )
	{
	case STATIC:  operand(  "Z"  ); break;
	case POSTINC: operand(  "Z+" ); break;
	case PREDEC:  operand( "-Z"  ); break;
	default:      operand( "???" ); break;
	}
}

/***********************************************************
 * Load full register from Y
 *	Encoding:
 *  15       8 7       0
 *   ---- ---r rrrr --mm
 ************************************************************/
OPERAND_FUNC(r_Ypm)
{
	enum {
		STATIC  = 0x00,
		POSTINC = 0x01,
		PREDEC  = 0x02
	} mode = opc & 0x03;
	
	operand_rD5( f, addr, opc, xtype );
	COMMA;
	switch ( mode )
	{
	case STATIC:  operand(  "Y"  ); break;
	case POSTINC: operand(  "Y+" ); break;
	case PREDEC:  operand( "-Y"  ); break;
	default:      operand( "???" ); break;
	}
}

/***********************************************************
 * Load full register from X
 *	Encoding:
 *  15       8 7       0
 *   ---- ---r rrrr --mm
 ************************************************************/
OPERAND_FUNC(r_Xpm)
{
	enum {
		STATIC  = 0x00,
		POSTINC = 0x01,
		PREDEC  = 0x02
	} mode = opc & 0x03;
	
	operand_rD5( f, addr, opc, xtype );
	COMMA;
	switch ( mode )
	{
	case STATIC:  operand(  "X"  ); break;
	case POSTINC: operand(  "X+" ); break;
	case PREDEC:  operand( "-X"  ); break;
	default:      operand( "???" ); break;
	}
}

/***********************************************************
 * Store full register to Z
 *	Encoding:
 *  15       8 7       0
 *   ---- ---r rrrr --mm
 ************************************************************/
OPERAND_FUNC(Zpm_r)
{
	enum {
		STATIC  = 0x00,
		POSTINC = 0x01,
		PREDEC  = 0x02
	} mode = opc & 0x03;
	
	switch ( mode )
	{
	case STATIC:  operand(  "Z"  ); break;
	case POSTINC: operand(  "Z+" ); break;
	case PREDEC:  operand( "-Z"  ); break;
	default:      operand( "???" ); break;
	}
	COMMA;
	operand_rD5( f, addr, opc, xtype );
}

/***********************************************************
 * Store full register to Y
 *	Encoding:
 *  15       8 7       0
 *   ---- ---r rrrr --mm
 ************************************************************/
OPERAND_FUNC(Ypm_r)
{
	enum {
		STATIC  = 0x00,
		POSTINC = 0x01,
		PREDEC  = 0x02
	} mode = opc & 0x03;
	
	switch ( mode )
	{
	case STATIC:  operand(  "Y"  ); break;
	case POSTINC: operand(  "Y+" ); break;
	case PREDEC:  operand( "-Y"  ); break;
	default:      operand( "???" ); break;
	}
	COMMA;
	operand_rD5( f, addr, opc, xtype );
}

/***********************************************************
 * Store full register to X
 *	Encoding:
 *  15       8 7       0
 *   ---- ---r rrrr --mm
 ************************************************************/
OPERAND_FUNC(Xpm_r)
{
	enum {
		STATIC  = 0x00,
		POSTINC = 0x01,
		PREDEC  = 0x02
	} mode = opc & 0x03;
	
	switch ( mode )
	{
	case STATIC:  operand(  "X"  ); break;
	case POSTINC: operand(  "X+" ); break;
	case PREDEC:  operand( "-X"  ); break;
	default:      operand( "???" ); break;
	}
	COMMA;
	operand_rD5( f, addr, opc, xtype );
}

/***********************************************************
 * Load from data space, encoded in two opcs.
 *  15       8 7       0    15                 0
 *   ---- ---r rrrr ---r     kkkk kkkk kkkk kkkk
 ************************************************************/
OPERAND_FUNC(r_k16)
{
	ADDR dest = (ADDR)nextw( f, addr );
	
	operand_rD5( f, addr, opc, xtype );
	COMMA;
	operand( xref_genwordaddr( NULL, "$", dest ) );
	xref_addxref( xtype, g_insn_addr, dest ); 
}

/***********************************************************
 * Store to data space, encoded in two opcs.
 *  15       8 7       0    15                 0
 *   ---- ---r rrrr ---r     kkkk kkkk kkkk kkkk
 ************************************************************/
OPERAND_FUNC(k16_r)
{
	ADDR dest = (ADDR)nextw( f, addr );
	
	operand( xref_genwordaddr( NULL, "$", dest ) );
	xref_addxref( xtype, g_insn_addr, dest ); 
	COMMA;
	operand_rD5( f, addr, opc, xtype );
}

/***********************************************************
 * Z and full width register field
 *	Encoding:
 *  15       8 7       0
 *   ---- ---r rrrr ---
 ************************************************************/
OPERAND_FUNC(Z_r)
{
	operand( "Z" );
	COMMA;
	operand_rD5( f, addr, opc, xtype );
}

/******************************************************************************/
/** Instruction Decoding Tables                                              **/
/** Note: tables are here as they refer to operand functions defined above.  **/
/******************************************************************************/

#define DUAL_REG(r)	(((r&0x1F)<<4)|(r&0x0F)|((r<<5)&0x200))
#define ALT_RR(M_name,M_mask)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x00),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x01),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x02),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x03),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x04),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x05),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x06),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x07),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x08),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x09),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x0A),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x0B),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x0C),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x0D),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x0E),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x0F),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x10),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x11),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x12),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x13),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x14),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x15),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x16),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x17),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x18),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x19),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x1A),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x1B),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x1C),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x1D),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x1E),X_NONE)	\
	INSN(M_name,rD5,M_mask|DUAL_REG(0x1F),X_NONE)
	

static optab_t base_optab[] = {

/* Note: listed in opcode numerical order, rather than grouped by function */

	INSN ( "NOP",    none,          0x0000,         X_NONE )
	
	MASK ( "MOVW",   rpair_rpair,   0xFF00, 0x0100, X_NONE )
	
	MASK ( "MULS",   rhigh_rhigh,   0xFF00, 0x0200, X_NONE )

	MASK ( "MULSU",  rhigh3_rhigh3, 0xFF88, 0x0300, X_NONE )
   MASK ( "FMUL",   rhigh3_rhigh3, 0xFF88, 0x0308, X_NONE )
	MASK ( "FMULS",  rhigh3_rhigh3, 0xFF88, 0x0380, X_NONE )
	MASK ( "FMULSU", rhigh3_rhigh3, 0xFF88, 0x0388, X_NONE )
	
	MASK ( "CPC",    r_r,           0xFC00, 0X0400, X_NONE )
	MASK ( "SBC",    r_r,           0xFC00, 0X0800, X_NONE )
	
	ALT_RR ( "LSL", 0x0C00 )
	MASK ( "ADD",    r_r,           0xFC00, 0X0C00, X_NONE )
	
	MASK ( "CPSE",   r_r,           0xFC00, 0X1000, X_NONE )	
	MASK ( "CP",     r_r,           0xFC00, 0X1400, X_NONE )	
	MASK ( "SUB",    r_r,           0xFC00, 0X1800, X_NONE )	
	
	ALT_RR ( "ROL", 0x1C00 )
	MASK ( "ADC",    r_r,           0xFC00, 0X1C00, X_NONE )
	
	ALT_RR ( "TST", 0x2000 )
	MASK ( "AND",    r_r,           0xFC00, 0X2000, X_NONE )

   ALT_RR ( "CLR", 0x2400 )	
	MASK ( "EOR",    r_r,           0xFC00, 0X2400, X_NONE )
	
	MASK ( "OR",     r_r,           0xFC00, 0X2800, X_NONE )	
	MASK ( "MOV",    r_r,           0xFC00, 0X2C00, X_NONE )
	
	MASK ( "CPI",    rhigh_k8,      0xF000, 0x3000, X_IMM )
	MASK ( "SBCI",   rhigh_k8,      0xF000, 0x4000, X_IMM )
	MASK ( "SUBI",   rhigh_k8,      0xF000, 0x5000, X_IMM )
	MASK ( "ORI",    rhigh_k8,      0xF000, 0x6000, X_IMM )
	MASK ( "ANDI",   rhigh_k8,      0xF000, 0x7000, X_IMM )
	
	MASK ( "LD",     r_YZ,          0xD200, 0x8000, X_NONE )
	MASK ( "ST",     YZ_r,          0xD200, 0x8200, X_NONE )
	
	MASK ( "LPM",    r_Zpm,         0xFE0F, 0x9004, X_NONE )
	MASK ( "LPM",    r_Zpm,         0xFE0F, 0x9005, X_NONE )
	
	MASK ( "ELPM",   r_Zpm,         0xFE0F, 0x9006, X_NONE )
	MASK ( "ELPM",   r_Zpm,         0xFE0F, 0x9007, X_NONE )
	
	MASK ( "POP",    rD5,           0xFE0F, 0x900F, X_NONE )
	MASK ( "PUSH",   rD5,           0xFE0F, 0x920F, X_NONE )
	
	MASK ( "LD",     r_Zpm,         0xFE0F, 0x9001, X_NONE )
	MASK ( "LD",     r_Zpm,         0xFE0F, 0x9002, X_NONE )
	MASK ( "LD",     r_Ypm,         0xFE0F, 0x9009, X_NONE )
	MASK ( "LD",     r_Ypm,         0xFE0F, 0x900A, X_NONE )
	MASK ( "LD",     r_Xpm,         0xFE0F, 0x900C, X_NONE )
	MASK ( "LD",     r_Xpm,         0xFE0F, 0x900D, X_NONE )
	MASK ( "LD",     r_Xpm,         0xFE0F, 0x900E, X_NONE )
	
	MASK ( "ST",     Zpm_r,         0xFE0F, 0x9201, X_NONE )
	MASK ( "ST",     Zpm_r,         0xFE0F, 0x9202, X_NONE )
	MASK ( "ST",     Ypm_r,         0xFE0F, 0x9209, X_NONE )
	MASK ( "ST",     Ypm_r,         0xFE0F, 0x920A, X_NONE )
	MASK ( "ST",     Xpm_r,         0xFE0F, 0x920C, X_NONE )
	MASK ( "ST",     Xpm_r,         0xFE0F, 0x920D, X_NONE )
	MASK ( "ST",     Xpm_r,         0xFE0F, 0x920E, X_NONE )
	
	MASK ( "LDS",    r_k16,         0xFE0F, 0x9000, X_PTR  )
	MASK ( "STS",    k16_r,         0xFE0F, 0x9200, X_PTR  )
	
	MASK ( "BCLR",   bit,           0xFF8F, 0x9488, X_NONE )
	MASK ( "BSET",   bit,           0xFF8F, 0x9408, X_NONE )
	
	MASK ( "COM",    rD5,           0xFE0F, 0x9400, X_NONE )
	MASK ( "NEG",    rD5,           0xFE0F, 0x9401, X_NONE )
	MASK ( "SWAP",   rD5,           0xFE0F, 0x9402, X_NONE )
	MASK ( "INC",    rD5,           0xFE0F, 0x9403, X_NONE )
	MASK ( "ASR",    rD5,           0xFE0F, 0x9405, X_NONE )
	MASK ( "LSR",    rD5,           0xFE0F, 0x9406, X_NONE )
	MASK ( "ROR",    rD5,           0xFE0F, 0x9407, X_NONE )
	MASK ( "DEC",    rD5,           0xFE0F, 0x940A, X_NONE )
	
	INSN ( "SEC",    none,          0x9408,         X_NONE )
	INSN ( "SEZ",    none,          0x9418,         X_NONE )
	INSN ( "SEN",    none,          0x9428,         X_NONE )
	INSN ( "SEV",    none,          0x9438,         X_NONE )
	INSN ( "SES",    none,          0x9448,         X_NONE )
	INSN ( "SEH",    none,          0x9458,         X_NONE )
	INSN ( "SET",    none,          0x9468,         X_NONE )
	INSN ( "SEI",    none,          0x9478,         X_NONE )
	
	INSN ( "CLC",    none,          0x9488,         X_NONE )
	INSN ( "CLZ",    none,          0x9498,         X_NONE )
	INSN ( "CLN",    none,          0x94A8,         X_NONE )
	INSN ( "CLV",    none,          0x94B8,         X_NONE )
	INSN ( "CLS",    none,          0x94C8,         X_NONE )
	INSN ( "CLH",    none,          0x94D8,         X_NONE )
	INSN ( "CLT",    none,          0x94E8,         X_NONE )
	INSN ( "CLI",    none,          0x94F8,         X_NONE )
	
	INSN ( "IJMP",   none,          0x9409,         X_NONE )
	INSN ( "EIJMP",  none,          0x9419,         X_NONE )
	INSN ( "RET",    none,          0x9508,         X_NONE )
	INSN ( "ICALL",  none,          0x9509,         X_NONE )
	INSN ( "RETI",   none,          0x9518,         X_NONE )
	INSN ( "EICALL", none,          0x9519,         X_NONE )
	
	INSN ( "SLEEP",  none,          0x9588,         X_NONE )
	INSN ( "BREAK",  none,          0x9598,         X_NONE )
	INSN ( "WDR",    none,          0x95A8,         X_NONE )
	
	MASK ( "CALL",   long_addr,     0xFE0E, 0x940E, X_CALL )
	MASK ( "JMP",    long_addr,     0xFE0E, 0x940C, X_JMP  )
	
	INSN ( "LPM",    none,          0x95C8,         X_NONE )
	INSN ( "ELPM",   none,          0x95D8,         X_NONE )
	INSN ( "SPM",    none,          0x95E8,         X_NONE )
	
	MASK ( "ADIW",   rphigh_k6,     0xFF00, 0x9600, X_IMM )
	MASK ( "SBIW",   rphigh_k6,     0xFF00, 0x9700, X_IMM )
	
	MASK ( "CBI",    A_b,           0xFC00, 0x9800, X_NONE )
	MASK ( "SBIC",   A_b,           0xFC00, 0x9900, X_NONE )
	MASK ( "SBI",    A_b,           0xFC00, 0x9A00, X_NONE )
	MASK ( "SBIS",   A_b,           0xFC00, 0x9B00, X_NONE )
	
	MASK ( "MUL",    r_r,           0xFC00, 0x9C00, X_NONE )
	
	MASK ( "IN",     r_A6,          0xF800, 0xB000, X_NONE )
	MASK ( "OUT",    A6_r,          0xF800, 0xB800, X_NONE )
	
	MASK ( "RJMP",   rel_k12,       0xF000, 0xC000, X_JMP  )
	MASK ( "RCALL",  rel_k12,       0xF000, 0xD000, X_CALL )
	
	MASK ( "LDI",    rhigh_k8,      0xF000, 0xE000, X_IMM )
	
	MASK ( "BRCS",   bra7,          0xFC07, 0xF000, X_JMP )
	MASK ( "BRCC",   bra7,          0xFC07, 0xF400, X_JMP )
	MASK ( "BREQ",   bra7,          0xFC07, 0xF001, X_JMP )
	MASK ( "BRNE",   bra7,          0xFC07, 0xF401, X_JMP )
	MASK ( "BRMI",   bra7,          0xFC07, 0xF002, X_JMP )
	MASK ( "BRPL",   bra7,          0xFC07, 0xF402, X_JMP )
	MASK ( "BRVS",   bra7,          0xFC07, 0xF003, X_JMP )
	MASK ( "BRVC",   bra7,          0xFC07, 0xF403, X_JMP )
	MASK ( "BRLT",   bra7,          0xFC07, 0xF004, X_JMP )
	MASK ( "BRGE",   bra7,          0xFC07, 0xF404, X_JMP )
	MASK ( "BRHS",   bra7,          0xFC07, 0xF005, X_JMP )
	MASK ( "BRHC",   bra7,          0xFC07, 0xF405, X_JMP )
	MASK ( "BRTS",   bra7,          0xFC07, 0xF006, X_JMP )
	MASK ( "BRTC",   bra7,          0xFC07, 0xF406, X_JMP )
	MASK ( "BRIE",   bra7,          0xFC07, 0xF007, X_JMP )
	MASK ( "BRID",   bra7,          0xFC07, 0xF407, X_JMP )
	
	MASK ( "BLD",    r_b,           0xFE08, 0xF800, X_NONE )
	MASK ( "BST",    r_b,           0xFE08, 0xFA00, X_NONE )
	MASK ( "SBRC",   r_b,           0xFE08, 0xFC00, X_NONE )
	MASK ( "SBRS",   r_b,           0xFE08, 0xFE00, X_NONE )
  
/*----------------------------------------------------------------------------*/  
	
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
 *      INSN_FOUND if a valid instruction found.
 *      INSN_NOT_FOUND otherwise.
 *
 ************************************************************/

static int walk_table( FILE * f, ADDR * addr, optab_t * optab, UWORD opc )
{
	if ( optab == NULL )
		return 0;
		
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
					 ( optab->type == OPTAB_RANGE 
					 		&& opc >= optab->u.range.min 
							&& opc <= optab->u.range.max )
					||
					 ( optab->type == OPTAB_MASK 
					 		&& ( ( opc & optab->u.mask.mask ) == optab->u.mask.val ) ) )
		{
			opcode( optab->opcode );
			optab->operands( f, addr, opc, optab->xtype );
			return INSN_FOUND;
		}

		optab++;	
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
	OPC opc;
	int found = 0;

	/* Store start address in a global for use in xref calls */	
	g_insn_addr = addr;
	
	/* Setup g_output_buffer to point to caller's output buffer */
	g_output_buffer = outbuf;

	/* Get first opcode */
	opc = nextw( f, &addr );

	/* Now walk table(s) looking for an instruction match */
	found = walk_table( f, &addr, base_optab, opc );
	
	/* If we didn't find a match, indicate this to the output */
	if ( found != INSN_FOUND )
		opcode( "???" );
	
	return addr;
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
