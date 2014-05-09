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
const char * dasm_name            = "dasm09";

/* Decoder description */
const char * dasm_description     = "Motorola 6809";

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
 * Process "#imm8" operands.
 *	byte comes from next byte.
 ************************************************************/

OPERAND_FUNC(imm8)
{
   UBYTE byte = next( f, addr );
	
	operand( "#" FORMAT_NUM_8BIT, byte );
}

/***********************************************************
 * Process "imm16" operand.
 ************************************************************/

OPERAND_FUNC(imm16)
{
	UBYTE msb   = next( f, addr );
	UBYTE lsb   = next( f, addr );
	UWORD imm16 = MK_WORD( lsb, msb );

	operand( xref_genwordaddr( NULL, "$", imm16 ) );
	xref_addxref( xtype, g_insn_addr, imm16 );
}



OPERAND_FUNC(direct)
{
	UBYTE a = next( f, addr );
	
	operand( xref_genwordaddr( NULL, "$", (ADDR)a ) );
	xref_addxref( xtype, g_insn_addr, a );
}

#define IND_OPEN(ind)		do{if(ind)operand("[");}while(0)
#define IND_CLOSE(ind)	do{if(ind)operand("]");}while(0)

OPERAND_FUNC(indexed)
{
	UBYTE postbyte = next( f, addr );
	UBYTE rr = ( postbyte >> 5 ) & 0x03;
	static const char * rrtab[] = { "X", "Y", "U", "S" };
	
	if ( postbyte & BIT(7) )
	{
		BYTE offset = ( ( postbyte & 0x1F ) << 3 ) >> 3;
		
		operand( "%d, %s", offset, rrtab[rr] );	
	}
	else
	{
		UBYTE mode = postbyte & 0x0F;
		int   ind  = postbyte & 0x10;
		
		switch ( mode )
		{
		case 0x00:
			operand( ",%s+", rrtab[rr] );
			break;
			
		case 0x01:
			IND_OPEN(ind);
			operand( ",%s++", rrtab[rr] );
			IND_CLOSE(ind);
			break;
			
		case 0x02:
			operand( ",-%s", rrtab[rr] );
			break;
			
		case 0x03:
			IND_OPEN(ind);
			operand( ",--%s", rrtab[rr] );
			IND_CLOSE(ind);
			break;
			
		}	
	}
}

OPERAND_FUNC(extended)
{
	UBYTE msb    = next( f, addr );
	UBYTE lsb    = next( f, addr );
	UWORD addr16 = MK_WORD( lsb, msb );

	operand( xref_genwordaddr( NULL, "$", addr16 ) );
	xref_addxref( xtype, g_insn_addr, addr16 );
}

/***********************************************************
 * Process "rel8" operands.
 ************************************************************/

OPERAND_FUNC(rel8)
{
	BYTE disp = (BYTE)next( f, addr );
	ADDR dest = *addr + disp;
	
	operand( xref_genwordaddr( NULL, "$", dest ) );
	xref_addxref( xtype, g_insn_addr, dest );
}

/***********************************************************
 * Process "rel16" operands.
 ************************************************************/

OPERAND_FUNC(rel16)
{
	UBYTE msb = next( f, addr );
	UBYTE lsb = next( f, addr );
	WORD disp = MK_WORD( lsb, msb );
	ADDR dest = *addr + disp;
	
	operand( xref_genwordaddr( NULL, "$", dest ) );
	xref_addxref( xtype, g_insn_addr, dest );
}



OPERAND_FUNC(r1_r2)
{
	UBYTE postbyte = next( f, addr );
	int   src = ( postbyte >> 4 ) & 0x0F;
	int   dst =   postbyte        & 0x0F;
	
	static const char * rtab[] = {
		"D",
		"X",
		"Y",
		"U",
		"S",
		"PC",
		"", "",
		"A",
		"B",
		"CCR",
		"DPR"
	};
	
	operand( "%s", rtab[dst] );
	COMMA;
	operand( "%s", rtab[src] );
}

/******************************************************************************/
/** Instruction Decoding Tables                                              **/
/** Note: tables are here as they refer to operand functions defined above.  **/
/******************************************************************************/

static optab_t page2[] = {

	INSN ( "LBRN", rel16, 0x21, X_JMP )
	INSN ( "LBHI", rel16, 0x22, X_JMP )
	INSN ( "LBLS", rel16, 0x23, X_JMP )
	
	INSN ( "LBCC", rel16, 0x24, X_JMP )
	INSN ( "LBCS", rel16, 0x25, X_JMP )
	/* Two alternate namings */
	INSN ( "LBHS", rel16, 0x24, X_JMP )
	INSN ( "LBLO", rel16, 0x25, X_JMP )
	
	INSN ( "LBNE", rel16, 0x26, X_JMP )
	INSN ( "LBEQ", rel16, 0x27, X_JMP )
	INSN ( "LBVC", rel16, 0x28, X_JMP )
	INSN ( "LBVS", rel16, 0x29, X_JMP )
	INSN ( "LBPL", rel16, 0x2A, X_JMP )
	INSN ( "LBMI", rel16, 0x2B, X_JMP )
	INSN ( "LBGE", rel16, 0x2C, X_JMP )
	INSN ( "LBLT", rel16, 0x2D, X_JMP )
	INSN ( "LBGT", rel16, 0x2E, X_JMP )
	INSN ( "LBLE", rel16, 0x2F, X_JMP )

	INSN ( "SWI2", none,  0x3F, X_NONE )
	
	INSN ( "CMPD", imm16, 0x83, X_NONE )
	INSN ( "CMPY", imm16, 0x8C, X_NONE )
	INSN ( "LDY",  imm16, 0x8E, X_NONE )
	
	INSN ( "CMPD", direct, 0x93, X_NONE )
	INSN ( "CMPY", direct, 0x9C, X_NONE )
	INSN ( "LDY",  direct, 0x9E, X_NONE )
	INSN ( "STY",  direct, 0x9F, X_NONE )
	
	INSN ( "CMPD", indexed, 0xA3, X_NONE )
	INSN ( "CMPY", indexed, 0xAC, X_NONE )
	INSN ( "LDY",  indexed, 0xAE, X_NONE )
	INSN ( "STY",  indexed, 0xAF, X_NONE )

	INSN ( "CMPD", extended, 0xB3, X_NONE )
	INSN ( "CMPY", extended, 0xBC, X_NONE )
	INSN ( "LDY",  extended, 0xBE, X_NONE )
	INSN ( "STY",  extended, 0xBF, X_NONE )
	
	INSN ( "LDS", imm16,  0xCE, X_NONE )
	INSN ( "LDS", direct, 0xDE, X_NONE )
	INSN ( "STS", direct, 0xDF, X_NONE )
	
	INSN ( "LDS", indexed, 0xEE, X_NONE )
	INSN ( "STS", indexed, 0xEF, X_NONE )

	INSN ( "LDS", extended, 0xFE, X_NONE )
	INSN ( "STS", extended, 0xFF, X_NONE )
	
	END
};

static optab_t page3[] = {

	INSN ( "SWI3", none,     0x3F, X_NONE )

	INSN ( "CMPU", imm16,    0x83, X_NONE )
	INSN ( "CMPS", imm16,    0x8C, X_NONE )

	INSN ( "CMPU", direct,   0x93, X_NONE )
	INSN ( "CMPS", direct,   0x9C, X_NONE )

	INSN ( "CMPU", indexed,  0xA3, X_NONE )
	INSN ( "CMPS", indexed,  0xAC, X_NONE )

	INSN ( "CMPU", extended, 0xB3, X_NONE )
	INSN ( "CMPS", extended, 0xBC, X_NONE )

	END
};

static optab_t base_optab[] = {

/*----------------------------------------------------------------------------
  8-bit Accumulator and Memory
  ----------------------------------------------------------------------------*/
  
#define ACC_ARGS_OP(M_name, M_base)	\
		INSN(M_name, imm8,     (0x80 | M_base), X_NONE) \
		INSN(M_name, direct,   (0x90 | M_base), X_NONE) \
		INSN(M_name, indexed,  (0xA0 | M_base), X_NONE) \
		INSN(M_name, extended, (0xB0 | M_base), X_NONE)
		
#define ACC_ARGS_OPD(M_name, M_base)	\
		INSN(M_name, imm16,    (0xC0 | M_base), X_NONE) \
		INSN(M_name, direct,   (0xD0 | M_base), X_NONE) \
		INSN(M_name, indexed,  (0xE0 | M_base), X_NONE) \
		INSN(M_name, extended, (0xF0 | M_base), X_NONE)		
		
	ACC_ARGS_OP( "SUBD", 0x03 )
	ACC_ARGS_OP( "CMPX", 0x0C )
	ACC_ARGS_OP( "LDX",  0x0E )
	
	ACC_ARGS_OPD( "ADDD", 0x03 )
	ACC_ARGS_OPD( "LDD",  0x0C )
	ACC_ARGS_OPD( "LDU",  0x0E )
	


#define ACC_AB_ARGS_OP(M_name, M_base)	\
		ACC_ARGS_OP(M_name "A", (0x00 | M_base)) \
		ACC_ARGS_OP(M_name "B", (0x40 | M_base))
		
	
	ACC_AB_ARGS_OP( "ADC", 0x09 )
	ACC_AB_ARGS_OP( "ADD", 0x0B )
	ACC_AB_ARGS_OP( "AND", 0x04 )
	ACC_AB_ARGS_OP( "BIT", 0x05 )
	ACC_AB_ARGS_OP( "CMP", 0x01 )
	ACC_AB_ARGS_OP( "EOR", 0x08 )
	ACC_AB_ARGS_OP( "LD",  0x06 )
	ACC_AB_ARGS_OP( "OR",  0x0A )
	ACC_AB_ARGS_OP( "SBC", 0x02 )
	ACC_AB_ARGS_OP( "SUB", 0x00 )
	
	/* Single Operator */
	
#define SINGLE_OP(M_name, M_base) \
		INSN(M_name, direct,   (0x00 | M_base), X_NONE) \
		INSN(M_name, indexed,  (0x60 | M_base), X_NONE) \
		INSN(M_name, extended, (0x70 | M_base), X_NONE)
		
	SINGLE_OP( "ASL", 0x08 )
	SINGLE_OP( "ASR", 0x07 )
	SINGLE_OP( "CLR", 0x0F )
	SINGLE_OP( "COM", 0x03 )
	SINGLE_OP( "DEC", 0x0A )
	SINGLE_OP( "INC", 0x0C )
	SINGLE_OP( "LSL", 0x08 ) /* Note same opcodes as for ASLA/B */
	SINGLE_OP( "LSR", 0x04 )
	SINGLE_OP( "NEG", 0x00 )
	SINGLE_OP( "ROL", 0x09 )
	SINGLE_OP( "ROR", 0x06 )
	SINGLE_OP( "TST", 0x0D )
	SINGLE_OP( "JMP", 0x0E )
	
	
	
	
	
	
	
	/* INHERENT */
	
	INSN ( "ABX",  none, 0x3A, X_NONE )
	INSN ( "DAA",  none, 0x19, X_NONE )
	INSN ( "MUL",  none, 0x3D, X_NONE )
	INSN ( "NOP",  none, 0x12, X_NONE )
	INSN ( "RTI",  none, 0x3B, X_NONE )
	INSN ( "RTS",  none, 0x39, X_NONE )
	INSN ( "SEX",  none, 0x1D, X_NONE )
	INSN ( "SWI",  none, 0x3F, X_NONE )
	INSN ( "SYNC", none, 0x13, X_NONE )
	
	/* All the inherent A/B insns */
	
#define ACC_OP_INH(M_name, M_base) \
		INSN( M_name "A", none, ( 0x40 | M_base ), X_NONE ) \
		INSN( M_name "B", none, ( 0x50 | M_base ), X_NONE )	
	
	ACC_OP_INH( "ASL", 0x08 )
	ACC_OP_INH( "ASR", 0x07 )
	ACC_OP_INH( "CLR", 0x0F )
	ACC_OP_INH( "COM", 0x03 )
	ACC_OP_INH( "DEC", 0x0A )
	ACC_OP_INH( "INC", 0x0C )
	ACC_OP_INH( "LSL", 0x08 ) /* Note same opcodes as for ASLA/B */
	ACC_OP_INH( "LSR", 0x04 )
	ACC_OP_INH( "NEG", 0x00 )
	ACC_OP_INH( "ROL", 0x09 )
	ACC_OP_INH( "ROR", 0x06 )
	ACC_OP_INH( "TST", 0x0D )
	
	
	INSN ( "BRA", rel8, 0x20, X_JMP )
	INSN ( "BRN", rel8, 0x21, X_JMP )
	INSN ( "BHI", rel8, 0x22, X_JMP )
	INSN ( "BLS", rel8, 0x23, X_JMP )
	
	INSN ( "BCC", rel8, 0x24, X_JMP )
	INSN ( "BCS", rel8, 0x25, X_JMP )
	/* Two alternate namings */
	INSN ( "BHS", rel8, 0x24, X_JMP )
	INSN ( "BLO", rel8, 0x25, X_JMP )
	
	INSN ( "BNE", rel8, 0x26, X_JMP )
	INSN ( "BEQ", rel8, 0x27, X_JMP )
	INSN ( "BVC", rel8, 0x28, X_JMP )
	INSN ( "BVS", rel8, 0x29, X_JMP )
	INSN ( "BPL", rel8, 0x2A, X_JMP )
	INSN ( "BMI", rel8, 0x2B, X_JMP )
	INSN ( "BGE", rel8, 0x2C, X_JMP )
	INSN ( "BLT", rel8, 0x2D, X_JMP )
	INSN ( "BGT", rel8, 0x2E, X_JMP )
	INSN ( "BLE", rel8, 0x2F, X_JMP )
	
	
	
	
	
	
	
	INSN ( "CWAI", imm8, 0x3C, X_NONE )
	
	
	INSN ( "LEAX", indexed, 0x30, X_NONE )
	INSN ( "LEAY", indexed, 0x31, X_NONE )
	INSN ( "LEAS", indexed, 0x32, X_NONE )
	INSN ( "LEAU", indexed, 0x33, X_NONE )
	INSN ( "PHSH", imm8, 0x34, X_NONE )
	INSN ( "PULS", imm8, 0x35, X_NONE )
	INSN ( "PSHU", imm8, 0x36, X_NONE )
	INSN ( "PULU", imm8, 0x37, X_NONE )
	
	
	INSN ( "LBRA", rel16, 0x16, X_JMP )
	INSN ( "LBSR", rel16, 0x17, X_JMP )
	
	INSN ( "ORCC",  imm8, 0x1A, X_NONE )
	INSN ( "ANDCC", imm8, 0x1C, X_NONE )
	
	INSN ( "EXG",   r1_r2, 0x1E, X_NONE )
	INSN ( "TFR",   r1_r2, 0x1F, X_NONE )
	
	
	
	
	INSN ( "BSR", rel8, 0x8D, X_JMP )
	
#define ACC_ARGS_OP_NOIMM(M_name, M_base, M_xref)	\
		INSN(M_name, direct,   (0x90 | M_base), M_xref) \
		INSN(M_name, indexed,  (0xA0 | M_base), M_xref) \
		INSN(M_name, extended, (0xB0 | M_base), M_xref)	
	
	ACC_ARGS_OP_NOIMM( "JSR", 0x0D, X_CALL )
	ACC_ARGS_OP_NOIMM( "STA", 0x07, X_PTR )
	ACC_ARGS_OP_NOIMM( "STX", 0x0F, X_PTR )
	
	ACC_ARGS_OP_NOIMM( "STB", 0x47, X_PTR )
	ACC_ARGS_OP_NOIMM( "STD", 0x4D, X_PTR )
	ACC_ARGS_OP_NOIMM( "STU", 0x4F, X_PTR )
	
/*----------------------------------------------------------------------------
  16-bit Accumulator and Memory
  ----------------------------------------------------------------------------*/



  
/*----------------------------------------------------------------------------
  Index Register/Stack Pointer
  ----------------------------------------------------------------------------*/




/*----------------------------------------------------------------------------
  Branch
  ----------------------------------------------------------------------------*/
  

  
  
  
/*----------------------------------------------------------------------------
  Miscellaneous
  ----------------------------------------------------------------------------*/
  
  
  
  
  

/*----------------------------------------------------------------------------
  Sub-Pages
  ----------------------------------------------------------------------------*/  
  
  TABLE ( page2, 0x10 )
  TABLE ( page3, 0x11 )
  
/*----------------------------------------------------------------------------*/  
  
  
  
#if 0  

#undef ACC_OP
#define ACC_OP(M_name, M_base)		\
   INSN ( M_name, imm8,       ( 0x09 | M_base ), X_NONE ) \
	INSN ( M_name, zeropage,   ( 0x05 | M_base ), X_PTR  ) \
	INSN ( M_name, zeropage_X, ( 0x15 | M_base ), X_PTR  ) \
	INSN ( M_name, abs16,      ( 0x0D | M_base ), X_PTR  ) \
	INSN ( M_name, abs16_X,    ( 0x1D | M_base ), X_PTR  ) \
	INSN ( M_name, abs16_Y,    ( 0x19 | M_base ), X_PTR  ) \
	INSN ( M_name, ind8_X,     ( 0x01 | M_base ), X_PTR  ) \
	INSN ( M_name, ind8_Y,     ( 0x11 | M_base ), X_PTR  )

/*----------------------------------------------------------------------------
  Load/Store
  ----------------------------------------------------------------------------*/

	ACC_OP( "lda", 0xA0 )
	
	INSN ( "ldx", imm8,       0xA2, X_NONE )
	INSN ( "ldx", zeropage,   0xA6, X_PTR  )
	INSN ( "ldx", zeropage_Y, 0xB6, X_PTR  )
	INSN ( "ldx", abs16,      0xAE, X_PTR  )
	INSN ( "ldx", abs16_Y,    0xBE, X_PTR  )
	
   INSN ( "ldy", imm8,       0xA0, X_NONE )
	INSN ( "ldy", zeropage,   0xA4, X_PTR  )
	INSN ( "ldy", zeropage_X, 0xB4, X_PTR  )
	INSN ( "ldy", abs16,      0xAC, X_PTR  )
	INSN ( "ldy", abs16_X,    0xBC, X_PTR  )
	
	INSN ( "sta", zeropage,   0x85, X_PTR  )
	INSN ( "sta", zeropage_X, 0x95, X_PTR  )
	INSN ( "sta", abs16,      0x8D, X_PTR  )
	INSN ( "sta", abs16_X,    0x9D, X_PTR  )
	INSN ( "sta", abs16_Y,    0x99, X_PTR  )
	INSN ( "sta", ind8_X,     0x81, X_PTR  )
	INSN ( "sta", ind8_Y,     0x91, X_PTR  )
	
	INSN ( "stx", zeropage,   0x86, X_PTR  )
	INSN ( "stx", zeropage_Y, 0x96, X_PTR  )
	INSN ( "stx", abs16,      0x8E, X_PTR  )
	
	INSN ( "sty", zeropage,   0x84, X_PTR  )
	INSN ( "sty", zeropage_X, 0x94, X_PTR  )
	INSN ( "sty", abs16,      0x8C, X_PTR  )
	
/*----------------------------------------------------------------------------
  Register Transfers
  ----------------------------------------------------------------------------*/
  
   INSN ( "tax",  none,      0xAA, X_NONE )
   INSN ( "tay",  none,      0xA8, X_NONE )
   INSN ( "txa",  none,      0x8A, X_NONE )
   INSN ( "tya",  none,      0x98, X_NONE )
  
/*----------------------------------------------------------------------------
  Stack Operations
  ----------------------------------------------------------------------------*/
  
	INSN ( "tsx",  none,      0xBA, X_NONE )
	INSN ( "txs",  none,      0x9A, X_NONE )
	INSN ( "pha",  none,      0x48, X_NONE )
	INSN ( "php",  none,      0x08, X_NONE )
	INSN ( "pla",  none,      0x68, X_NONE )
	INSN ( "plp",  none,      0x28, X_NONE )
	
/*----------------------------------------------------------------------------
  Logical
  ----------------------------------------------------------------------------*/

   ACC_OP( "and", 0x20 )
	ACC_OP( "eor", 0x40 )
   ACC_OP( "ora", 0x00 )
	
	INSN ( "bit", zeropage,   0x24, X_PTR  )
	INSN ( "bit", abs16,      0x2C, X_PTR  )
	
/*----------------------------------------------------------------------------
  Arithmetic
  ----------------------------------------------------------------------------*/
 
	ACC_OP( "adc", 0x60 )
	ACC_OP( "sbc", 0xE0 )
	ACC_OP( "cmp", 0xC0 )
	
	INSN ( "cpx", imm8,       0xE0, X_NONE )
	INSN ( "cpx", zeropage,   0xE4, X_PTR  )
	INSN ( "cpx", abs16,      0xEC, X_PTR  )
	
	INSN ( "cpy", imm8,       0xC0, X_NONE )
	INSN ( "cpy", zeropage,   0xC4, X_PTR  )
	INSN ( "cpy", abs16,      0xCC, X_PTR  )
	
/*----------------------------------------------------------------------------
  Increment/Decrement
  ----------------------------------------------------------------------------*/	
	
	INSN ( "inc", zeropage,   0xE6, X_PTR  )
	INSN ( "inc", zeropage_X, 0xF6, X_PTR  )
	INSN ( "inc", abs16,      0xEE, X_PTR  )
	INSN ( "inc", abs16_X,    0xFE, X_PTR  )
	
	INSN ( "inx", none,       0xE8, X_NONE )
	INSN ( "iny", none,       0xC8, X_NONE )
	
	INSN ( "dec", zeropage,   0xC6, X_PTR  )
	INSN ( "dec", zeropage_X, 0xD6, X_PTR  )
	INSN ( "dec", abs16,      0xCE, X_PTR  )
	INSN ( "dec", abs16_X,    0xDE, X_PTR  )
	
	INSN ( "dex", none,       0xCA, X_NONE )
	INSN ( "dey", none,       0x88, X_NONE )
	
/*----------------------------------------------------------------------------
  Shift and Rotate
  ----------------------------------------------------------------------------*/	
	
#undef ACC_OP
#define ACC_OP(M_name, M_base)		\
   INSN ( M_name, none,       ( 0x0A | M_base ), X_NONE ) \
	INSN ( M_name, zeropage,   ( 0x06 | M_base ), X_PTR  ) \
	INSN ( M_name, zeropage_X, ( 0x16 | M_base ), X_PTR  ) \
	INSN ( M_name, abs16,      ( 0x0E | M_base ), X_PTR  ) \
	INSN ( M_name, abs16_X,    ( 0x1E | M_base ), X_PTR  )
	
	ACC_OP( "asl", 0x00 )
	ACC_OP( "lsr", 0x40 )
	ACC_OP( "rol", 0x20 )
	ACC_OP( "ror", 0x60 )	
	
/*----------------------------------------------------------------------------
  Jumps and Calls
  ----------------------------------------------------------------------------*/
  
   INSN ( "jmp", abs16,      0x4C, X_JMP  )
	INSN ( "jmp", ind16,      0x6C, X_PTR  )
   INSN ( "jsr", abs16,      0x20, X_CALL )
   INSN ( "rts", none,       0x60, X_NONE )
	
/*----------------------------------------------------------------------------
  Conditional Branch
  ----------------------------------------------------------------------------*/	
	
   INSN  ( "bcc",    rel8, 0x90, X_JMP )
	INSN  ( "bcs",    rel8, 0xB0, X_JMP )
	INSN  ( "beq",    rel8, 0xF0, X_JMP )
	INSN  ( "bmi",    rel8, 0x30, X_JMP )
	INSN  ( "bne",    rel8, 0xD0, X_JMP )
	INSN  ( "bpl",    rel8, 0x10, X_JMP )
	INSN  ( "bvc",    rel8, 0x50, X_JMP )
	INSN  ( "bvs",    rel8, 0x70, X_JMP )
	
/*----------------------------------------------------------------------------
  Status Flag Changes
  ----------------------------------------------------------------------------*/
	
	INSN ( "clc",     none, 0x18, X_NONE )
	INSN ( "cld",     none, 0xD8, X_NONE )
	INSN ( "cli",     none, 0x58, X_NONE )
	INSN ( "clv",     none, 0xB8, X_NONE )
	INSN ( "sec",     none, 0x38, X_NONE )
	INSN ( "sed",     none, 0xF8, X_NONE )
	INSN ( "sei",     none, 0x78, X_NONE )
	
/*----------------------------------------------------------------------------
  System Functions
  ----------------------------------------------------------------------------*/
	
	INSN ( "brk",     none, 0x00, X_NONE )
	INSN ( "nop",     none, 0xEA, X_NONE )
	INSN ( "rti",     none, 0x40, X_NONE )

#endif
	
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
		else if ( optab->type == OPTAB_MEMMOD  /* NEC78k3 */
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
