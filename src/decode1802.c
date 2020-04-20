/*****************************************************************************
 *
 * Copyright (C) 2019, Neil Johnson
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
 * 
 * Instruction disassembler for the RCA CDP1802
 * 
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

DASM_PROFILE( "dasm1802", "RCA CDP1802", 3, 9, 0, 1 )

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/

/* Common output formats */
#define FORMAT_NUM_8BIT         "$%02X"
#define FORMAT_NUM_16BIT        "$%04X"
#define FORMAT_REG              "%X"

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
 * Process register operands.
 *    register comes from bottom 4 bits of the opcode
 ************************************************************/

OPERAND_FUNC(reg)
{
    int reg = opc & 0x0F;
    
    operand( FORMAT_REG, reg );    
}

/***********************************************************
 * Process "page8" operands.
 ************************************************************/

OPERAND_FUNC(page8)
{
    UBYTE aa = next( f, addr );
    ADDR dest = ( *addr & 0xFF00 ) | aa;
    
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

/***********************************************************
 * Process "io" operand.
 ************************************************************/

OPERAND_FUNC(io)
{
    int ionum = ( opc & 0x07 );
    
    operand( FORMAT_REG, ionum );
}

/***********************************************************
 * Process "addr16" operand.
 ************************************************************/

OPERAND_FUNC(addr16)
{
    UBYTE low_addr  = next( f, addr );
    UBYTE high_addr = next( f, addr );
    UWORD addr16    = MK_WORD( low_addr, high_addr );

    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr16 ) );
    xref_addxref( xtype, g_insn_addr, addr16 );
}

/******************************************************************************/
/** Instruction Decoding Tables                                              **/
/** Note: tables are here as they refer to operand functions defined above.  **/
/******************************************************************************/

optab_t base_optab[] = {
    
    /* 0X */
    INSN ( "IDL",   none, 0x00, X_NONE )
    MASK ( "LDN",   reg, 0xF0, 0x00, X_NONE )
    
    /* 1X */
    MASK ( "INC",   reg, 0xF0, 0x10, X_NONE )
    
    /* 2X */
    MASK ( "DEC",   reg, 0xF0, 0x20, X_NONE )
    
    /* 3X */
    INSN ( "BR",    page8, 0x30, X_JMP )
    INSN ( "BQ",    page8, 0x31, X_JMP )
    INSN ( "BZ",    page8, 0x32, X_JMP )
    INSN ( "BDF",   page8, 0x33, X_JMP )
    INSN ( "B1",    page8, 0x34, X_JMP )
    INSN ( "B2",    page8, 0x35, X_JMP )
    INSN ( "B3",    page8, 0x36, X_JMP )
    INSN ( "B4",    page8, 0x37, X_JMP )
    INSN ( "SKP",   none, 0x38, X_JMP )
    INSN ( "BNQ",   page8, 0x39, X_JMP )
    INSN ( "BNZ",   page8, 0x3A, X_JMP )
    INSN ( "BNF",   page8, 0x3B, X_JMP )
    INSN ( "BN1",   page8, 0x3C, X_JMP )
    INSN ( "BN2",   page8, 0x3D, X_JMP )
    INSN ( "BN3",   page8, 0x3E, X_JMP )
    INSN ( "BN4",   page8, 0x3F, X_JMP )
    
    /* 4X */
    MASK ( "LDA",   reg, 0xF0, 0x40, X_NONE )
    
    /* 5X */
    MASK ( "STR",   reg, 0xF0, 0x50, X_NONE )
    
    /* 6X */
    INSN ( "IRX",   none, 0x60, X_NONE )
    RANGE( "OUT",   io, 0x61, 0x67, X_IO )
    INSN ( "?????", none, 0x68, X_NONE )
    RANGE( "INP",   io, 0x69, 0x6F, X_IO )
    
    /* 7X */
    INSN ( "RET",   none, 0x70, X_NONE )
    INSN ( "DIS",   none, 0x71, X_NONE )
    INSN ( "LDXA",  none, 0x72, X_NONE )
    INSN ( "STXD",  none, 0x73, X_NONE )
    INSN ( "ADC",   none, 0x74, X_NONE )
    INSN ( "SDB",   none, 0x75, X_NONE )
    INSN ( "SHRC",  none, 0x76, X_NONE )
    INSN ( "SMB",   none, 0x77, X_NONE )
    INSN ( "SAV",   none, 0x78, X_NONE )
    INSN ( "MARK",  none, 0x79, X_NONE )
    INSN ( "REQ",   none, 0x7A, X_NONE )
    INSN ( "SEQ",   none, 0x7B, X_NONE )
    
    INSN ( "ADCI",  imm8, 0x7C, X_IMM )
    INSN ( "SBDI",  imm8, 0x7D, X_IMM )
    INSN ( "SHLC",  none, 0x7E, X_NONE )
    INSN ( "SMBI",  imm8, 0x7F, X_IMM )
    
    /* 8X */
    MASK ( "GLO",   reg, 0xF0, 0x80, X_NONE )
    
    /* 9X */
    MASK ( "GHI",   reg, 0xF0, 0x90, X_NONE )
    
    /* AX */
    MASK ( "PLO",   reg, 0xF0, 0xA0, X_NONE )
    
    /* BX */
    MASK ( "PHI",   reg, 0xF0, 0xB0, X_NONE )
    
    /* CX */
    INSN ( "LBR",   addr16, 0xC0, X_JMP )
    INSN ( "LBQ",   addr16, 0xC1, X_JMP )
    INSN ( "LBZ",   addr16, 0xC2, X_JMP )
    INSN ( "LBDF",  addr16, 0xC3, X_JMP )
    INSN ( "NOP",   none, 0xC4, X_NONE )
    INSN ( "LSNQ",  none, 0xC5, X_NONE )
    INSN ( "LSNZ",  none, 0xC6, X_NONE )
    INSN ( "LSNF",  none, 0xC7, X_NONE )
    INSN ( "LSKP",  none, 0xC8, X_NONE )
    INSN ( "LBNQ",  addr16, 0xC9, X_JMP )
    INSN ( "LBNZ",  addr16, 0xCA, X_JMP )
    INSN ( "LBNF",  addr16, 0xCB, X_JMP )
    INSN ( "LSIE",  none, 0xCC, X_NONE )
    INSN ( "LSQ",   none, 0xCD, X_NONE )
    INSN ( "LSZ",   none, 0xCE, X_NONE )
    INSN ( "LSDF",  none, 0xCF, X_NONE )
    
    /* DX */
    MASK ( "SEP",   reg, 0xF0, 0xD0, X_NONE )
    
    /* EX */
    MASK ( "SEX",   reg, 0xF0, 0xE0, X_NONE )
    
    /* FX */
    INSN ( "LDX",   none, 0xF0, X_NONE )
    INSN ( "OR",    none, 0xF1, X_NONE )
    INSN ( "AND",   none, 0xF2, X_NONE )
    INSN ( "XOR",   none, 0xF3, X_NONE )
    INSN ( "ADD",   none, 0xF4, X_NONE )
    INSN ( "SD",    none, 0xF5, X_NONE )
    INSN ( "SHR",   none, 0xF6, X_NONE )
    INSN ( "SM",    none, 0xF7, X_NONE )
    INSN ( "LDI",   imm8, 0xF8, X_IMM )
    INSN ( "ORI",   imm8, 0xF9, X_IMM )
    INSN ( "ANI",   imm8, 0xFA, X_IMM )
    INSN ( "XRI",   imm8, 0xFB, X_IMM )
    INSN ( "ADI",   imm8, 0xFC, X_IMM )
    INSN ( "SDI",   imm8, 0xFD, X_IMM )
    INSN ( "SHL",   none, 0xFE, X_NONE )
    INSN ( "SMI",   imm8, 0xFF, X_IMM )

    END
};

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
