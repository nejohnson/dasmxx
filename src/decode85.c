/*****************************************************************************
 *
 * Copyright (C) 2014-2019, Neil Johnson
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
 *   8085 INSTRUCTION SET (including undocumented instructions)
 * 
 * Note that the undocumented flag is denoted "K" per 
 *   http://www.righto.com/2013/02/looking-at-silicon-to-understanding.html
 * rather than the "X5" as used by Dehnhardt and Sorensen.
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "dasmxx.h"
#include "optab.h"

/*****************************************************************************
 * Globally-visible decoder properties
 *****************************************************************************/

DASM_PROFILE( "dasm85", "Intel 8085", 4, 9, 0, 1, 1 )

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/

/* Common output formats */
#define FORMAT_NUM_8BIT         "0%02XH"
#define FORMAT_NUM_16BIT        "0%04XH"

/* Construct a 16-bit word out of low and high bytes */
#define MK_WORD(l,h)            ( ((l) & 0xFF) | (((h) & 0xFF) << 8) )

/*****************************************************************************
 *        Private Functions
 *****************************************************************************/

/******************************************************************************/
/**                            Operand Functions                             **/
/******************************************************************************/

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
 *    byte comes from next byte.
 ************************************************************/

OPERAND_FUNC(imm8)
{
    UBYTE byte = next( f, addr );
    
    operand( FORMAT_NUM_8BIT, byte );
}

/***********************************************************
 * Process "imm16" operand.
 ************************************************************/

OPERAND_FUNC(imm16)
{
    UBYTE lsb   = next( f, addr );
    UBYTE msb   = next( f, addr );
    UWORD imm16 = MK_WORD( lsb, msb );

    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, imm16 ) );
    xref_addxref( xtype, g_insn_addr, imm16 );
}

/***********************************************************
 * Register operand within opcode
 ************************************************************/

/* xxxx_xRRR */
OPERAND_FUNC(regS)
{
    UBYTE reg = opc & 0x07;
    static char *rtab[] = { "B", "C", "D", "E", "H", "L", "M", "A" };
    
    operand( "%s", rtab[reg] );
}

/* xxRRR_Rxxx */
OPERAND_FUNC(regD)
{
    operand_regS( f, addr, opc >> 3, xtype );
}

/* xxRR_xxxx */
OPERAND_FUNC(rpairPSW)
{
    UBYTE reg = ( opc >> 4 ) & 0x03;
    static char *rtab[] = { "B", "D", "H", "PSW" };
    
    operand( "%s", rtab[reg] );
}

OPERAND_FUNC(rpair)
{
    UBYTE reg = ( opc >> 4 ) & 0x03;
    static char *rtab[] = { "B", "D", "H", "SP" };
    
    operand( "%s", rtab[reg] );
}

/* xxRR_xxxx */
OPERAND_FUNC(indrpair)
{
    UBYTE reg = ( opc >> 4 ) & 0x03;
    static char *rtab[] = { "B", "D", "???", "???" };
    
    operand( "%s", rtab[reg] );
}

/***********************************************************
 * Process absolute memory address operands.
 ************************************************************/

OPERAND_FUNC(addr16)
{
    UBYTE lsb = next( f, addr );
    UBYTE msb = next( f, addr );
    ADDR dest = MK_WORD( lsb, msb );
    
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

/***********************************************************
 * Process absolute memory indirect address operands.
 ************************************************************/

OPERAND_FUNC(mem16)
{
    UBYTE lsb = next( f, addr );
    UBYTE msb = next( f, addr );
    ADDR dest = MK_WORD( lsb, msb );
    
    operand( "(%s)", xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

/***********************************************************
 * Process IO address operands.
 ************************************************************/

OPERAND_FUNC(iop8)
{
    UBYTE ioport = next( f, addr );
    
    operand( "(%s)", xref_genwordaddr( NULL, FORMAT_NUM_8BIT, ioport ) );
    xref_addxref( xtype, g_insn_addr, ioport );
}

/***********************************************************
 * Process RST (restart) operands.
 ************************************************************/

OPERAND_FUNC(rst)
{
    UBYTE rst = ( opc >> 3 ) & 0x07;
    
    operand( FORMAT_NUM_8BIT, rst );
}

/******************************************************************************/
/**                            Double Operands                               **/
/******************************************************************************/

TWO_OPERAND(regD, regS)
TWO_OPERAND(regD, imm8)
TWO_OPERAND(rpair, imm16)

/******************************************************************************/
/** Instruction Decoding Tables                                              **/
/** Note: tables are here as they refer to operand functions defined above.  **/
/******************************************************************************/

/******************************************************************************/
/** Base table **/
/******************************************************************************/

optab_t base_optab[] = {
    
    INSN ( "HLT",   none,        0x76,       X_NONE )   /* important order */
    
/*----------------------------------------------------------------------------
  Data Transfer
  ----------------------------------------------------------------------------*/
  
    MASK ( "MOV",   regD_regS,   0xC0, 0x40, X_PTR )
    MASK ( "MVI",   regD_imm8,   0xC7, 0x06, X_IMM ) 

    MASK ( "LXI",   rpair_imm16, 0xCF, 0x01, X_IMM )
  
    MASK ( "LDAX",  indrpair,    0xEF, 0x0A, X_PTR )
    MASK ( "STAX",  indrpair,    0xEF, 0x02, X_PTR )
    
    INSN ( "LDA",   mem16,       0x3A,       X_PTR )  
    INSN ( "STA",   mem16,       0x32,       X_PTR )
    
    INSN ( "LHLD",  mem16,       0x2A,       X_PTR )
    INSN ( "SHLD",  mem16,       0x22,       X_PTR )
    
    INSN ( "LDHI",  imm8,        0x28,       X_NONE )   /* undoc */
    INSN ( "LDSI",  imm8,        0x38,       X_NONE )   /* undoc */
    
    INSN ( "XCHG",  none,        0xEB,       X_NONE )
    
    INSN ( "LHLX",  none,        0xED,       X_NONE )   /* undoc */
    INSN ( "SHLX",  none,        0xD9,       X_NONE )   /* undoc */

/*----------------------------------------------------------------------------
  Data Manipulation - Arithmetic
  ----------------------------------------------------------------------------*/
  
    MASK ( "ADD",   regS,        0xF8, 0x80, X_NONE )
    MASK ( "ADC",   regS,        0xF8, 0x88, X_NONE )
    MASK ( "SUB",   regS,        0xF8, 0x90, X_NONE )
    MASK ( "SBB",   regS,        0xF8, 0x98, X_NONE )
  
    MASK ( "DAD",   rpair,       0xCF, 0x09, X_NONE )
    INSN ( "DSUB",  rpair,       0x08,       X_NONE )   /* undoc */
  
    INSN ( "ADI",   imm8,        0xC6,       X_IMM )
    INSN ( "ACI",   imm8,        0xCE,       X_IMM )
    INSN ( "SUI",   imm8,        0xD6,       X_IMM )
    INSN ( "SBI",   imm8,        0xDE,       X_IMM )
  
    MASK ( "INR",   regD,        0xC7, 0x04, X_REG )
    MASK ( "DCR",   regD,        0xC7, 0x05, X_REG )  
        
    INSN ( "DAA",   none,        0x27,       X_NONE )
    INSN ( "CMA",   none,        0x2F,       X_NONE )
    INSN ( "STC",   none,        0x37,       X_NONE )
    INSN ( "CMC",   none,        0x3F,       X_NONE )
    
    MASK ( "INX",   rpair,       0xCF, 0x03, X_NONE )
    MASK ( "DCX",   rpair,       0xCF, 0x0B, X_NONE )
    
/*----------------------------------------------------------------------------
  Data Manipulation - Logical
  ----------------------------------------------------------------------------*/    

    MASK ( "ANA",   regS,        0xF8, 0xA0, X_NONE )
    MASK ( "ORA",   regS,        0xF8, 0xB0, X_NONE )
    MASK ( "XRA",   regS,        0xF8, 0xA8, X_NONE )
    MASK ( "CPA",   regS,        0xF8, 0xB8, X_NONE )
        
    INSN ( "RLC",   none,        0x07,       X_NONE )
    INSN ( "RRC",   none,        0x0F,       X_NONE )
    INSN ( "RAL",   none,        0x17,       X_NONE )
    INSN ( "RAR",   none,        0x1F,       X_NONE )
    
    INSN ( "ARHL",  none,        0x10,       X_NONE )   /* undoc */
    INSN ( "RDEL",  none,        0x18,       X_NONE )   /* undoc */
    
    INSN ( "ANI",   imm8,        0xE6,       X_IMM )
    INSN ( "ORI",   imm8,        0xF6,       X_IMM )
    INSN ( "XRI",   imm8,        0xEE,       X_IMM )
    INSN ( "CPI",   imm8,        0xFE,       X_IMM )
    
/*----------------------------------------------------------------------------
  Control Transfer
  ----------------------------------------------------------------------------*/

    INSN ( "JMP",   addr16,      0xC3,       X_JMP )
    
#define JCR(M_name,M_op,M_bit,M_type) \
    INSN( M_name "NZ",M_op, M_bit | (0 << 3), M_type) \
    INSN( M_name "Z", M_op, M_bit | (1 << 3), M_type) \
    INSN( M_name "NC",M_op, M_bit | (2 << 3), M_type) \
    INSN( M_name "C", M_op, M_bit | (3 << 3), M_type) \
    INSN( M_name "PO",M_op, M_bit | (4 << 3), M_type) \
    INSN( M_name "PE",M_op, M_bit | (5 << 3), M_type) \
    INSN( M_name "P", M_op, M_bit | (6 << 3), M_type) \
    INSN( M_name "M", M_op, M_bit | (7 << 3), M_type)
    
    JCR  ( "J",     addr16,      0xC2,       X_JMP )
    INSN ( "JNK",   addr16,      0xDD,       X_JMP )    /* undoc */
    INSN ( "JK",    addr16,      0xFD,       X_JMP )    /* undoc */

    INSN ( "CALL",  addr16,      0xCD,       X_CALL )
    JCR  ( "C",     addr16,      0xC4,       X_CALL )

    INSN ( "RET",   none,        0xC9,       X_NONE )
    JCR  ( "R",     none,        0xC0,       X_NONE )

    INSN ( "PCHL",  none,        0xE9,       X_NONE )

/*----------------------------------------------------------------------------
  Stack Operations
  ----------------------------------------------------------------------------*/

    MASK ( "PUSH",  rpairPSW,    0xCF, 0xC5, X_REG )
    MASK ( "POP",   rpairPSW,    0xCF, 0xC1, X_REG )

    INSN ( "XTHL",  none,        0xE3,       X_NONE )
    INSN ( "SPHL",  none,        0xF9,       X_NONE )

/*----------------------------------------------------------------------------
  Machine Control
  ----------------------------------------------------------------------------*/

    INSN ( "NOP",   none,        0x00,       X_NONE )

    INSN ( "DI",    none,        0xF3,       X_NONE )
    INSN ( "EI",    none,        0xFB,       X_NONE )
    INSN ( "RIM",   none,        0x20,       X_NONE )
    INSN ( "SIM",   none,        0x30,       X_NONE )

    MASK ( "RST",   rst,         0xC7, 0xC7, X_NONE )
    
    INSN ( "RSTV",  none,        0xCB,       X_NONE )   /* undoc */

    INSN ( "IN",    iop8,        0xDB,       X_IO )
    INSN ( "OUT",   iop8,        0xD3,       X_IO )

/*----------------------------------------------------------------------------*/  
    
    END
};

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
