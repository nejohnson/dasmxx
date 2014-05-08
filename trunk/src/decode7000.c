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

/*****************************************************************************
 * Globally-visible decoder properties
 *****************************************************************************/

/* Decoder short name */
const char * dasm_name            = "dasm7000";

/* Decoder description */
const char * dasm_description     = "**EXPERIMENTAL**  TI TMS7000";

/* Decoder maximum instruction length in bytes */
const int    dasm_max_insn_length = 4;

/* Decoder maximum opcode field width */
const int    dasm_max_opcode_width = 9;

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/

/* Common output formats */
#define FORMAT_NUM_8BIT		"$%02X"
#define FORMAT_NUM_16BIT		"$%04X"
#define FORMAT_REG				"R%d"

/* Create a single-bit mask */
#define BIT(n)					( 1 << (n) )

/* Construct a 16-bit word out of low and high bytes */
#define MK_WORD(l,h)			( ((l) & 0xFF) | (((h) & 0xFF) << 8) )

/* Indicate whether decode was successful or not. */
#define INSN_FOUND				( 1 )
#define INSN_NOT_FOUND		( 0 )

/* Neaten up emitting a comma "," within an operand. */
#define COMMA	operand( ", " )

/**
	The optab_t type describes each entry in the op tables.
**/
typedef struct optab_s {
	UBYTE opc;
	const char * opcode;
	void (*operands)( FILE *, ADDR *, UBYTE, XREF_TYPE); /* operand function */
	XREF_TYPE xtype;
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
	A MASK2 matches a set of instruction bytes described by a bit mask and a
   value to match against applied to the second search byte.
**/	
#define MASK2(M_opcode, M_ops, M_opc, M_mask, M_val, M_xt) 							\
															{	.type     = OPTAB_MASK2,			\
																.opcode   = M_opcode,				\
																.opc      = M_opc, 				\
																.operands = operand_ ## M_ops, \
																.xtype    = M_xt,					\
																.u.mask.mask = M_mask,			\
																.u.mask.val  = M_val				\
															},
															
/**
	A MEMMOD describes an instruction with four memory-modifer combinations
	which must be decoded together for a prospective match.
**/	
#define MEMMOD(M_opcode, M_ops, M_opc, M_xt) 	{	.type     = OPTAB_MEMMOD,		\
																.opcode   = M_opcode,				\
																.opc      = M_opc, 				\
																.operands = operand_ ## M_ops,	\
																.xtype    = M_xt					\
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
	static void operand_ ## M_name (FILE *f, ADDR * addr, UBYTE opc, XREF_TYPE xtype )

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
 * "A" register.
 ************************************************************/

OPERAND_FUNC(A)
{
	operand( "A" );
}

/***********************************************************
 * "B" register.
 ************************************************************/

OPERAND_FUNC(B)
{
	operand( "B" );
}

/***********************************************************
 * "ST" stack pointer register.
 ************************************************************/
 
OPERAND_FUNC(ST)
{
	operand( "ST" );
}

/***********************************************************
 * General register.
 *	reg.num comes from next byte.
 ************************************************************/

OPERAND_FUNC(reg)
{
   UBYTE reg = next( f, addr );
	
	operand( FORMAT_REG, reg );
}

/***********************************************************
 * 8-bit Immediate operand
 *	value comes from next byte.
 ************************************************************/

OPERAND_FUNC(iop)
{
   UBYTE iop = next( f, addr );
	
	operand( "%%" FORMAT_NUM_8BIT, iop );
}

/***********************************************************
 * Peripheral file (I/O register).
 *	Register number comes from next byte.
 * Peripheral registers 00-FF are mapped into memory locations
 * 100-1FF.
 ************************************************************/
 
#define MAX_INTERNAL_PERIP_REG		( 11 )
#define INTERNAL_PERIP_REG_BASE	( 0x100 )
 
OPERAND_FUNC(Pn)
{
   UBYTE pn = next( f, addr );
	const char * s;
	
	if ( pn <= MAX_INTERNAL_PERIP_REG 
	     && ( s = xref_findaddrlabel( pn + INTERNAL_PERIP_REG_BASE ) ) )
		operand( "%s", s );
	else
		operand( "P" FORMAT_NUM_8BIT, pn );
}

/***********************************************************
 * TRAp instruction encodes the trap number in the opcode.
 ************************************************************/

OPERAND_FUNC(trap)
{
	UBYTE t = opc - 0xE8;
	
	operand( "%d", t );
}

/***********************************************************
 * 16-bit Immediate operand
 *	value comes from next two bytes.
 ************************************************************/
 
OPERAND_FUNC(iop16)
{
	UBYTE msb   = next( f, addr );
	UBYTE lsb   = next( f, addr );
	UWORD iop16 = MK_WORD( lsb, msb );

	operand( "%%%s", xref_genwordaddr( NULL, "$", iop16 ) );
	xref_addxref( xtype, g_insn_addr, iop16 );
}

/***********************************************************
 * 16-bit Immediate operand with B register added
 *	value comes from next two bytes.
 ************************************************************/
 
OPERAND_FUNC(iop16_B)
{
	operand_iop16( f, addr, opc, xtype );
	operand( "(B)" );
}

/***********************************************************
 * 16-bit address
 *	Address comes from next two bytes.
 ************************************************************/
 
OPERAND_FUNC(label)
{
	UBYTE msb_addr  = next( f, addr );
	UBYTE lsb_addr  = next( f, addr );
	UWORD addr16    = MK_WORD( lsb_addr, msb_addr );

	operand( "@%s", xref_genwordaddr( NULL, "$", addr16 ) );
	xref_addxref( xtype, g_insn_addr, addr16 );
}

/***********************************************************
 * 16-bit address indexed by B register.
 *	Address comes from next two bytes.
 ************************************************************/
 
OPERAND_FUNC(label_B)
{
	operand_label( f, addr, opc, xtype );
	operand( "(B)" );
}

/***********************************************************
 * Indirect register pair (lsb register specified).
 ************************************************************/
 
OPERAND_FUNC(indreg)
{
	operand( "*" );
	operand_reg( f, addr, opc, xtype );
}

/***********************************************************
 * Relative "ofst" operand.
 ************************************************************/

OPERAND_FUNC(ofst)
{
	BYTE ofst = (BYTE)next( f, addr );
	ADDR dest = *addr + ofst;
	
	operand( xref_genwordaddr( NULL, "$", dest ) );
	xref_addxref( xtype, g_insn_addr, dest );
}

/******************************************************************************/
/**                            Double Operands                               **/
/******************************************************************************/

OPERAND_FUNC(A_B)
{
	operand_A( f, addr, opc, xtype );
	COMMA;
	operand_B( f, addr, opc, xtype );
}

OPERAND_FUNC(B_A)
{
	operand_B( f, addr, opc, xtype );
	COMMA;
	operand_A( f, addr, opc, xtype );
}

OPERAND_FUNC(reg_A)
{
	operand_reg( f, addr, opc, xtype );
	COMMA;
	operand_A( f, addr, opc, xtype );
}

OPERAND_FUNC(reg_B)
{
	operand_reg( f, addr, opc, xtype );
	COMMA;
	operand_B( f, addr, opc, xtype );
}

OPERAND_FUNC(A_reg)
{
	operand_A( f, addr, opc, xtype );
	COMMA;
	operand_reg( f, addr, opc, xtype );
}

OPERAND_FUNC(B_reg)
{
	operand_B( f, addr, opc, xtype );
	COMMA;
	operand_reg( f, addr, opc, xtype );
}

OPERAND_FUNC(reg_reg)
{
	operand_reg( f, addr, opc, xtype );
	COMMA;
	operand_reg( f, addr, opc, xtype );
}

OPERAND_FUNC(iop_A)
{
	operand_iop( f, addr, opc, xtype );
	COMMA;
	operand_A( f, addr, opc, xtype );
}

OPERAND_FUNC(iop_B)
{
	operand_iop( f, addr, opc, xtype );
	COMMA;
	operand_B( f, addr, opc, xtype );
}

OPERAND_FUNC(iop_reg)
{
	operand_iop( f, addr, opc, xtype );
	COMMA;
	operand_reg( f, addr, opc, xtype );
}

OPERAND_FUNC(A_Pn)
{
	operand_A( f, addr, opc, xtype );
	COMMA;
	operand_Pn( f, addr, opc, xtype );
}

OPERAND_FUNC(B_Pn)
{
	operand_B( f, addr, opc, xtype );
	COMMA;
	operand_Pn( f, addr, opc, xtype );
}

OPERAND_FUNC(Pn_A)
{
	operand_Pn( f, addr, opc, xtype );
	COMMA;
	operand_A( f, addr, opc, xtype );
}

OPERAND_FUNC(Pn_B)
{
	operand_Pn( f, addr, opc, xtype );
	COMMA;
	operand_B( f, addr, opc, xtype );
}

OPERAND_FUNC(iop_Pn)
{
	operand_iop( f, addr, opc, xtype );
	COMMA;
	operand_Pn( f, addr, opc, xtype );
}

OPERAND_FUNC(iop16_reg)
{
	operand_iop16( f, addr, opc, xtype );
	COMMA;
	operand_reg( f, addr, opc, xtype );
}

OPERAND_FUNC(iop16_B_reg)
{
	operand_iop16_B( f, addr, opc, xtype );
	COMMA;
	operand_reg( f, addr, opc, xtype );
}

OPERAND_FUNC(A_ofst)
{
	operand_A( f, addr, opc, xtype );
	COMMA;
	operand_ofst( f, addr, opc, xtype );
}

OPERAND_FUNC(B_ofst)
{
	operand_B( f, addr, opc, xtype );
	COMMA;
	operand_ofst( f, addr, opc, xtype );
}

OPERAND_FUNC(reg_ofst)
{
	operand_reg( f, addr, opc, xtype );
	COMMA;
	operand_ofst( f, addr, opc, xtype );
}

/******************************************************************************/
/**                            Triple Operands                               **/
/******************************************************************************/

OPERAND_FUNC(B_A_ofst)
{
	operand_B( f, addr, opc, xtype );
	COMMA;
	operand_A_ofst( f, addr, opc, xtype );
}

OPERAND_FUNC(reg_A_ofst)
{
	operand_reg( f, addr, opc, xtype );
	COMMA;
	operand_A_ofst( f, addr, opc, xtype );
}

OPERAND_FUNC(reg_B_ofst)
{
	operand_reg( f, addr, opc, xtype );
	COMMA;
	operand_B_ofst( f, addr, opc, xtype );
}

OPERAND_FUNC(reg_reg_ofst)
{
	operand_reg( f, addr, opc, xtype );
	COMMA;
	operand_reg_ofst( f, addr, opc, xtype );
}

OPERAND_FUNC(iop_A_ofst)
{
	operand_iop( f, addr, opc, xtype );
	COMMA;
	operand_A_ofst( f, addr, opc, xtype );
}

OPERAND_FUNC(iop_B_ofst)
{
	operand_iop( f, addr, opc, xtype );
	COMMA;
	operand_B_ofst( f, addr, opc, xtype );
}

OPERAND_FUNC(iop_reg_ofst)
{
	operand_iop( f, addr, opc, xtype );
	COMMA;
	operand_reg_ofst( f, addr, opc, xtype );
}

OPERAND_FUNC(A_Pn_ofst)
{
	operand_A( f, addr, opc, xtype );
	COMMA;
	operand_Pn( f, addr, opc, xtype );
	COMMA;
	operand_ofst( f, addr, opc, xtype );
}

OPERAND_FUNC(B_Pn_ofst)
{
	operand_B( f, addr, opc, xtype );
	COMMA;
	operand_Pn( f, addr, opc, xtype );
	COMMA;
	operand_ofst( f, addr, opc, xtype );
}

OPERAND_FUNC(reg_Pn_ofst)
{
	operand_reg( f, addr, opc, xtype );
	COMMA;
	operand_Pn( f, addr, opc, xtype );
	COMMA;
	operand_ofst( f, addr, opc, xtype );
}

/******************************************************************************/
/** Instruction Decoding Tables                                              **/
/** Note: tables are here as they refer to operand functions defined above.  **/
/******************************************************************************/

#define ARITH_OP(M_name, M_base)	\
				INSN ( M_name, B_A,     ( 0x60 | M_base ), X_NONE ) \
				INSN ( M_name, reg_A,   ( 0x10 | M_base ), X_NONE ) \
				INSN ( M_name, reg_B,   ( 0x30 | M_base ), X_NONE ) \
				INSN ( M_name, reg_reg, ( 0x40 | M_base ), X_NONE ) \
				INSN ( M_name, iop_A,   ( 0x20 | M_base ), X_REG ) \
				INSN ( M_name, iop_B,   ( 0x50 | M_base ), X_REG ) \
				INSN ( M_name, iop_reg, ( 0x70 | M_base ), X_REG )
				
#define ARITH_P_OP(M_name, M_base)  \
				INSN ( M_name, A_Pn,    ( 0x80 | M_base ), X_PTR ) \
				INSN ( M_name, B_Pn,    ( 0x90 | M_base ), X_PTR ) \
				INSN ( M_name, iop_Pn,  ( 0xA0 | M_base ), X_PTR )
				
#define ARITH_SINGLE_OP(M_name, M_base)  \
				INSN ( M_name, A,       ( 0xB0 | M_base ), X_NONE ) \
				INSN ( M_name, B,       ( 0xC0 | M_base ), X_NONE ) \
				INSN ( M_name, reg,     ( 0xD0 | M_base ), X_NONE )
				
#define WORD_OPS(M_name, M_mask) \
				INSN( M_name, label,   ( 0x80 | M_mask ), X_PTR ) \
				INSN( M_name, label_B, ( 0xA0 | M_mask ), X_PTR ) \
				INSN( M_name, indreg,  ( 0x90 | M_mask ), X_PTR )
				
#define BT_OP(M_name, M_mask) \
				INSN( M_name, B_A_ofst,        ( 0x60 | M_mask ), X_NONE ) \
				INSN( M_name, reg_A_ofst,      ( 0x10 | M_mask ), X_NONE ) \
				INSN( M_name, reg_B_ofst,      ( 0x30 | M_mask ), X_NONE ) \
				INSN( M_name, reg_reg_ofst,    ( 0x40 | M_mask ), X_NONE ) \
				INSN( M_name, iop_A_ofst,      ( 0x20 | M_mask ), X_NONE ) \
				INSN( M_name, iop_B_ofst,      ( 0x50 | M_mask ), X_NONE ) \
				INSN( M_name, iop_reg_ofst,    ( 0x70 | M_mask ), X_NONE ) \
				INSN( M_name "P", A_Pn_ofst,   ( 0x80 | M_mask ), X_NONE ) \
				INSN( M_name "P", B_Pn_ofst,   ( 0x90 | M_mask ), X_NONE ) \
				INSN( M_name "P", reg_Pn_ofst, ( 0xA0 | M_mask ), X_NONE )



static optab_t base_optab[] = {

/*----------------------------------------------------------------------------
  Arithmetic
  ----------------------------------------------------------------------------*/

	ARITH_OP( "ADC", 0x09 )
	ARITH_OP( "ADD", 0x08 )
	ARITH_OP( "DAC", 0x0E )
	ARITH_OP( "DSB", 0x0F )
	ARITH_OP( "MPY", 0x0C )
	ARITH_OP( "SBB", 0x0B )
	ARITH_OP( "SUB", 0x0A )
	
	ARITH_SINGLE_OP( "CLR",  0x05 )
	ARITH_SINGLE_OP( "DEC",  0x02 )
	ARITH_SINGLE_OP( "DECD", 0x0B )
	ARITH_SINGLE_OP( "INC",  0x03 )
	
/*----------------------------------------------------------------------------
  Branch and Jump
  ----------------------------------------------------------------------------*/

#ifdef INTERPRET_MACHINE_FLAGS
	INSN ( "JHS", ofst, 0xE3, X_JMP )
	INSN ( "JL",  ofst, 0xE7, X_JMP )
	INSN ( "JEQ", ofst, 0xE2, X_JMP )
	INSN ( "JNE", ofst, 0xE6, X_JMP )
	INSN ( "JGT", ofst, 0xE4, X_JMP )
	INSN ( "JGE", ofst, 0xE5, X_JMP )
#else	
	INSN ( "JC",  ofst, 0xE3, X_JMP )
	INSN ( "JNC", ofst, 0xE7, X_JMP )
	INSN ( "JZ",  ofst, 0xE2, X_JMP )
	INSN ( "JNZ", ofst, 0xE6, X_JMP )
	INSN ( "JP",  ofst, 0xE4, X_JMP )
	INSN ( "JPZ", ofst, 0xE5, X_JMP )
#endif

	INSN ( "JMP", ofst, 0xE0, X_JMP )
	WORD_OPS( "BR",   0x0C )
	
	WORD_OPS( "CALL", 0x0E )
	INSN ( "RETI", none, 0x0B, X_NONE )
	INSN ( "RETS", none, 0x0A, X_NONE )

	BT_OP( "BTJO", 0x06 )
	BT_OP( "BTJZ", 0x07 )
	
	INSN( "DJNZ", A_ofst,   0xBA, X_NONE )
	INSN( "DJNZ", B_ofst,   0xCA, X_NONE )
	INSN( "DJNZ", reg_ofst, 0xDA, X_NONE )

/*----------------------------------------------------------------------------
  Compare
  ----------------------------------------------------------------------------*/	

	ARITH_OP( "CMP", 0x0D )
	WORD_OPS( "CMPA", 0x0D )
	
	/* INSN ( "TSTA", none, 0xB0, X_NONE ) -- NOTE: has same opcode as CLRC */
	INSN ( "TSTB", none, 0xC1, X_NONE )

/*----------------------------------------------------------------------------
  Control
  ----------------------------------------------------------------------------*/

	INSN ( "CLRC", none, 0xB0, X_NONE )
	INSN ( "SETC", none, 0x07, X_NONE )
	
	INSN ( "DINT", none, 0x06, X_NONE )
	INSN ( "EINT", none, 0x05, X_NONE )
	INSN ( "IDLE", none, 0x01, X_NONE )
	
	INSN ( "NOP",  none, 0x00, X_NONE )
	
	RANGE ( "TRAP", trap, 0xE8, 0xFF, X_NONE )

/*----------------------------------------------------------------------------
  Load and Move
  ----------------------------------------------------------------------------*/
  
	ARITH_OP( "MOV", 0x02 )
	INSN ( "MOV", A_B, 0xC0, X_NONE )
	INSN ( "MOV", A_reg, 0xD0, X_NONE )
	INSN ( "MOV", B_reg, 0xD1, X_NONE )

	INSN ( "MOVD", iop16_reg, 0x88, X_NONE )
	INSN ( "MOVD", iop16_B_reg, 0xA8, X_NONE )
	INSN ( "MOVD", reg_reg, 0x98, X_NONE )

	ARITH_P_OP( "MOVP", 0x02 )
	INSN ( "MOVP", Pn_A, 0x80, X_PTR )
	INSN ( "MOVP", Pn_B, 0x91, X_PTR )
  
   WORD_OPS( "LDA",  0x0A )
	WORD_OPS( "STA",  0x0B )
	
	INSN ( "LDSP", none, 0x0D, X_NONE )
	INSN ( "STSP", none, 0x09, X_NONE )
	
	ARITH_SINGLE_OP( "POP",  0x09 )
	ARITH_SINGLE_OP( "PUSH", 0x08 )
	INSN ( "POP",  ST, 0x08, X_NONE )
	INSN ( "PUSH", ST, 0x0E, X_NONE )
	
	INSN ( "XCHB", A,   0xB6, X_NONE )
	INSN ( "XCHB", reg, 0xD6, X_NONE )
  
/*----------------------------------------------------------------------------
  Logical
  ----------------------------------------------------------------------------*/

   ARITH_OP( "AND", 0x03 )
	ARITH_OP( "OR",  0x04 )
	ARITH_OP( "XOR", 0x05 )
	
	ARITH_P_OP( "ANDP", 0x03 )
	ARITH_P_OP( "ORP",  0x04 )
	ARITH_P_OP( "XORP", 0x05 )
	
   ARITH_SINGLE_OP( "INV",  0x04 )
	
/*----------------------------------------------------------------------------
  Shift
  ----------------------------------------------------------------------------*/	

	ARITH_SINGLE_OP( "RL",   0x0E )
	ARITH_SINGLE_OP( "RLC",  0x0F )
	ARITH_SINGLE_OP( "RR",   0x0C )
	ARITH_SINGLE_OP( "RRC",  0x0D )
	ARITH_SINGLE_OP( "SWAP", 0x07 )

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

static int walk_table( FILE * f, ADDR * addr, optab_t * optab, UBYTE opc )
{
	UBYTE peek_byte;
	int have_peeked = 0;
	
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
				optab->operands( f, addr, opc, optab->xtype );
				return INSN_FOUND;
			}
		}
		else if ( optab->type == OPTAB_MEMMOD   /* NEC78k3 */
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
				optab->operands( f, addr, opc, optab->xtype );
				return INSN_FOUND;
			}		
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
	int opc;
	int found = 0;

	/* Store start address in a global for use in xref calls */	
	g_insn_addr = addr;
	
	/* Setup g_output_buffer to point to caller's output buffer */
	g_output_buffer = outbuf;

	/* Get first opcode byte */
	opc = next( f, &addr );

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
