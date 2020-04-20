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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "dasmxx.h"
#include "optab.h"

/*****************************************************************************
 * Globally-visible decoder properties
 *****************************************************************************/

DASM_PROFILE( "dasmx86", "Intel x86", 5, 9, 0, 1 )

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/

/* Common output formats */
#define FORMAT_NUM_8BIT      "0%02X"
#define FORMAT_NUM_16BIT     "0%04X"

/* Construct a 16-bit word out of low and high bytes */
#define MK_WORD(l,h)         ( ((l) & 0xFF) | (((h) & 0xFF) << 8) )

#define NOSEGPFX    ( 0 )
#define EMIT_SEG_PFX \
    if (segpfx) {operand("%s:", segreg[segpfx]); segpfx = NOSEGPFX;}

/*****************************************************************************
 * Private data.  Declare as static.
 *****************************************************************************/

static const char * const segreg[5]  = { "", "ES", "CS", "SS", "DS" };
static const char * const wordreg[8] = { "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI" };
static const char * const bytereg[8] = { "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH" };

static int segpfx = NOSEGPFX;

/*****************************************************************************
 *        Private Functions
 *****************************************************************************/

/******************************************************************************/
/**                            Prefix Functions                              **/
/******************************************************************************/

/*
 * Segment override prefix is of the form
 *        001_SS_110
 */
PREFIX_FUNC(pfx_seg)
{
    int reg = (opc >> 3) & 0x03;
    
    segpfx = reg + 1;
}

PREFIX_FUNC(pfx_rep)
{
    if ( opc & 1 )
        operand( "REP  " );
    else
        operand( "REPZ " );    
}

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

/* This operand just gobbles up the next byte with no effect */
OPERAND_FUNC(gobble)
{
    UBYTE unused = next( f, addr );
}

/******************************************************************************/
/**                            Single Operands                               **/
/******************************************************************************/

OPERAND_FUNC(dx)
{
    operand( "DX" );
}

OPERAND_FUNC(reg)
{
    int reg = opc & 0x07;
    int isword = opc & 0x08;
    
    operand( (isword ? wordreg : bytereg)[reg] );
}

OPERAND_FUNC(reg16)
{
    int reg = opc & 0x07;
    
    operand( wordreg[reg] );
}

OPERAND_FUNC(acc)
{
    int wordop = opc & 1;
    operand( wordop ? "AX" : "AL" );
}

OPERAND_FUNC(imm8)
{
    UBYTE byte = next( f, addr );
    
    operand( FORMAT_NUM_8BIT, byte );
}

OPERAND_FUNC(port8)
{
    UBYTE byte = next( f, addr );
    
    operand( FORMAT_NUM_8BIT, byte );
}

OPERAND_FUNC(disp8)
{
    BYTE disp = (BYTE)next( f, addr );
    ADDR dest = *addr + disp;
    
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

OPERAND_FUNC(imm16)
{
    UBYTE lsb   = next( f, addr );
    UBYTE msb   = next( f, addr );
    UWORD imm16 = MK_WORD( lsb, msb );

    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, imm16 ) );
    xref_addxref( xtype, g_insn_addr, imm16 );
}

OPERAND_FUNC(addr16)
{
    UBYTE lsb = next( f, addr );
    UBYTE msb = next( f, addr );
    ADDR dest = MK_WORD( lsb, msb );
    
    EMIT_SEG_PFX;
    operand( "%c[%s]", 
        opc & 1 ? 'W' : 'B', 
        xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

OPERAND_FUNC(disp16)
{
    UBYTE lsb = next( f, addr );
    UBYTE msb = next( f, addr );
    ADDR dest = *addr + MK_WORD( lsb, msb );
    
    EMIT_SEG_PFX;
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

OPERAND_FUNC(segoff)
{
    UBYTE offlo = next( f, addr );
    UBYTE offhi = next( f, addr );
    UBYTE seglo = next( f, addr );
    UBYTE seghi = next( f, addr );
    
    ADDR offset = MK_WORD( offlo, offhi );
    ADDR segment = MK_WORD( seglo, seghi );
    
    operand( FORMAT_NUM_16BIT ":" FORMAT_NUM_16BIT, segment, offset );    
}

OPERAND_FUNC(segmreg)
{
    UBYTE reg = ( opc >> 3 ) & 3;
    
    operand( segreg[reg+1] );
}

OPERAND_FUNC(modrm)
{
    UBYTE arg  = next( f, addr );
    int mod    = (arg >> 6) & 3;
    int reg    = (arg >> 3) & 7;
    int rm     = arg & 7;
    int wordop = opc & 1;
    int dir    = opc & 2;
    int isseg  = 0;
    int dest, src, action;
    enum { DO_REG, DO_ADDR };
    static char *eareg[8] = { "BX + SI", "BX + DI", "BP + SI", "BP + DI",
                              "SI", "DI", "BP", "BX" };
    
    /* Handle special-case opcodes */
    switch( opc )
    {
        case 0xC4: /* LES */
            wordop = 1;
            dir = 1;
            break;
            
        case 0x8D: /* LEA */
        case 0xC5: /* LDS */
            dir = 1;
            break;
            
        case 0x8E:  /* MOV to SEG */
        case 0x8C:  /* MOV from SEG */
            isseg  = 1;
            wordop = 1;
            break;
    }
    
    /* ------------------ */

    /* Some opcodes use the REG field to encode the instruction selection.  In
     * these cases there is no REG, just the ADDR.
     * */
    if ( opc == 0xFF || opc == 0xFE || opc == 0x8F || opc == 0xD0 || opc == 0xD1
         || opc == 0xD2 || opc == 0xD3 
         || opc == 0x80 || opc == 0x81 || opc == 0x83 
         || opc == 0xC6 || opc == 0xC7 
         || opc == 0xF6 || opc == 0xF7 
         || ((opc & 0xF8) == 0xD8) )
    {
        src = action = DO_ADDR;
    }
    else
    {
        dest = dir ? DO_REG : DO_ADDR;
        src  = dir ? DO_ADDR : DO_REG;
        action = dest;
    }
        
    do {
        switch ( action )
        {
        case DO_REG:
            if ( isseg )
                operand( segreg[reg+1] );
            else
                operand( (wordop ? wordreg : bytereg)[reg] );
            break;
                
        case DO_ADDR:
            switch( mod )
            {
                case 0: /* MOD = 00, DISP is 0, except if rm = 110 then 
                         *  EA is 16-bit DISP */
                    EMIT_SEG_PFX;
                    if ( rm == 6 )
                    {
                        UBYTE displo = next( f, addr );
                        UBYTE disphi = next( f, addr );
                        ADDR disp = MK_WORD( displo, disphi );
                        operand( FORMAT_NUM_16BIT, disp );
                    }
                    else
                    {
                        operand( "%c[%s]", wordop ? 'W' : 'B', eareg[rm] );
                    }                
                    break;
                    
                case 1: /* MOD = 01, DISP is 8-bit sign-extended */
                {
                    BYTE disp = (BYTE)next( f, addr );
                    EMIT_SEG_PFX;
                    operand( "%c[%s + " FORMAT_NUM_8BIT "]", 
                        wordop ? 'W' : 'B',
                        eareg[rm], 
                        disp );
                    break;
                }
                    
                case 2: /* MOD = 10, DISP is 16-bit signed */
                {
                    UBYTE displo = next( f, addr );
                    UBYTE disphi = next( f, addr );
                    ADDR disp = MK_WORD( displo, disphi );
                    EMIT_SEG_PFX;
                    operand( "%c[%s + " FORMAT_NUM_16BIT "]", 
                        wordop ? 'W' : 'B',
                        eareg[rm], 
                        disp );                
                    break;
                }
                    
                case 3: /* MOD = 11, r/m is treated as reg field */
                    operand( (wordop ? wordreg : bytereg)[rm] );                
                    break;
            }
            break;
        }
        
        if ( action == src )
            break;
        action = src;
        operand( ", " );
    }
    while ( 1 );
}

OPERAND_FUNC(modrmC)
{
    UBYTE clreg = opc & 2;
    
    operand_modrm( f, addr, opc, xtype );
    
    if ( clreg )
        operand( ", CL" );
    else
        operand( ", 1" );    
}

OPERAND_FUNC(modrmimm)
{
    UBYTE datalo, datahi;
    UWORD imm16;
    
    operand_modrm( f, addr, opc, xtype );
    operand( ", " );
    
    /* Some variations do not support sign-extended immediates */
    if ( (opc & 0xFE) == 0xC6 || (opc & 0xFE) == 0xF6 )
        opc &= 1;
    
    switch( opc & 3 )
    {
    case 0: /* s:w = 00 :: 8-bit immediate */
        datalo = next( f, addr );
        operand( FORMAT_NUM_8BIT, datalo );
        break;
        
    case 1: /* s:w = 01 :: 16-bit immediate */
        datalo = next( f, addr );
        datahi = next( f, addr );
        imm16 = MK_WORD( datalo, datahi );
        operand( FORMAT_NUM_16BIT, imm16 );
        break;
        
    case 3: /* s:w = 11 :: 8-bit sign-extended to 16-bit */
        imm16 = next( f, addr );
        if ( imm16 & 0x80 ) imm16 |= 0xFF00;
        operand( FORMAT_NUM_16BIT, imm16 );
        break;
    }
}

/******************************************************************************/
/**                            Double Operands                               **/
/******************************************************************************/

TWO_OPERAND_PAIR(acc, addr16)
TWO_OPERAND_PAIR(acc, port8)
TWO_OPERAND_PAIR(acc, dx)

TWO_OPERAND(acc, imm8)
TWO_OPERAND(acc, imm16)

TWO_OPERAND(reg, imm8)
TWO_OPERAND(reg, imm16)

/******************************************************************************/
/** Instruction Decoding Tables                                              **/
/** Note: tables are here as they refer to operand functions defined above.  **/
/******************************************************************************/

optab_t base_optab[] = {

/* This has to go first because it is actually a pseudonym for "XCHG AX,AX" */
    INSN( "NOP",    none,  0x90, X_NONE )
    
/*----------------------------------------------------------------------------
  DATA TRANSFER
  ----------------------------------------------------------------------------*/
  
    MASK( "MOV",    acc_addr16,  0xFE, 0xA0, X_NONE )
    MASK( "MOV",    addr16_acc,  0xFE, 0xA2, X_NONE )
    
    MASK( "MOV",    reg_imm8,    0xF8, 0xB0, X_NONE )
    MASK( "MOV",    reg_imm16,   0xF8, 0xB8, X_NONE )
    
    MASK( "MOV",    modrm,       0xFC, 0x88, X_NONE )
    MASK( "MOV",    modrm,       0xFD, 0x8C, X_NONE ) /* seg regs */
    
    MASK( "MOV",    modrmimm,    0xFE, 0xC6, X_NONE )    
    
    MASK( "IN",     acc_port8,   0xFE, 0xE4, X_NONE )
    MASK( "IN",     acc_dx,      0xFE, 0xEC, X_NONE )
        
    MASK( "OUT",    port8_acc,   0xFE, 0xE6, X_NONE )
    MASK( "OUT",    dx_acc,      0xFE, 0xEE, X_NONE )
    
    MASK( "PUSH",   reg16,       0xF8, 0x50, X_NONE )
    MASK( "PUSH",   segmreg,     0xE7, 0x06, X_NONE )
    MASK2( "PUSH",  modrm,       0xFF, 0x38, 0x30, X_NONE )
        
    MASK( "POP",    reg16,       0xF8, 0x58, X_NONE )
    MASK( "POP",    segmreg,     0xE7, 0x07, X_NONE )
    MASK2( "POP",   modrm,       0x8F, 0x38, 0x00, X_NONE )
    
    MASK( "XCHG",   reg16,       0xF8, 0x90, X_NONE )
    MASK( "XCHG",   modrm,       0xFE, 0x86, X_NONE )
  
    INSN( "XLAT",   none,        0xD7, X_NONE )
    INSN( "LEA",    modrm,       0x8D, X_NONE )
    INSN( "LDS",    modrm,       0xC5, X_NONE )
    INSN( "LES",    modrm,       0xC4, X_NONE )
    
    INSN( "LAHF",   none, 0x9F, X_NONE )
    INSN( "SAHF",   none, 0x9E, X_NONE )
    INSN( "PUSHF",  none, 0x9C, X_NONE )
    INSN( "POPF",   none, 0x9D, X_NONE )
  
/*----------------------------------------------------------------------------
  ARITHMETIC
  ----------------------------------------------------------------------------*/

#define ARITH_IMM_ACC(M_name, M_mask) \
    INSN(M_name,acc_imm8,M_mask,X_NONE ) \
    INSN(M_name,acc_imm16,M_mask|1,X_NONE )

    ARITH_IMM_ACC("ADD", 0x04)
    ARITH_IMM_ACC("ADC", 0x14)
    ARITH_IMM_ACC("SUB", 0x2C)
    ARITH_IMM_ACC("SBB", 0x1C)
    ARITH_IMM_ACC("CMP", 0x3C)
    ARITH_IMM_ACC("AND", 0x24)
    ARITH_IMM_ACC("TEST",0xA8)
    ARITH_IMM_ACC("OR",  0x0C)
    ARITH_IMM_ACC("XOR", 0x34)
    
#define ARITH_IMMX_RM(M_name,M_mask) \
    MASK2(M_name,modrmimm,0x80,0x38,M_mask,X_NONE) \
    MASK2(M_name,modrmimm,0x81,0x38,M_mask,X_NONE) \
    MASK2(M_name,modrmimm,0x83,0x38,M_mask,X_NONE)
    
    ARITH_IMMX_RM( "ADD", 0x00 )
    ARITH_IMMX_RM( "ADC", 0x80 )
    ARITH_IMMX_RM( "SUB", 0x28 )
    ARITH_IMMX_RM( "SBB", 0x18 )
    ARITH_IMMX_RM( "CMP", 0x38 )
    
#define ARITH_IMM_RM(M_name,M_mask) \
    MASK2(M_name,modrmimm,0x80,0x38,M_mask,X_NONE) \
    MASK2(M_name,modrmimm,0x81,0x38,M_mask,X_NONE)
    
    ARITH_IMM_RM( "AND", 0x20 )
    ARITH_IMM_RM( "OR",  0x08 )
    ARITH_IMM_RM( "XOR", 0x30 )

    MASK( "TEST",   modrmimm, 0xFE, 0xF6, X_NONE )    
    
    MASK( "ADD",    modrm, 0xFC, 0x00, X_NONE )
    MASK( "ADC",    modrm, 0xFC, 0x10, X_NONE )
    MASK( "SUB",    modrm, 0xFC, 0x28, X_NONE )
    MASK( "SBB",    modrm, 0xFC, 0x18, X_NONE )
    MASK( "CMP",    modrm, 0xFC, 0x38, X_NONE )
    MASK( "AND",    modrm, 0xFC, 0x20, X_NONE )
    MASK( "TEST",   modrm, 0xFC, 0x84, X_NONE )
    MASK( "OR",     modrm, 0xFC, 0x08, X_NONE )
    MASK( "XOR",    modrm, 0xFC, 0x30, X_NONE )
    
    MASK( "INC",    reg16, 0xF8, 0x40, X_NONE )
    MASK2( "INC",   modrm, 0xFF, 0x38, 0x00, X_NONE )
    MASK2( "INC",   modrm, 0xFE, 0x38, 0x00, X_NONE )
    
    MASK( "DEC",    reg16, 0xF8, 0x48, X_NONE )
    MASK2( "DEC",   modrm, 0xFE, 0x38, 0x08, X_NONE )
    MASK2( "DEC",   modrm, 0xFF, 0x38, 0x08, X_NONE )
    
    MASK2( "NEG",   modrm, 0xFE, 0x38, 0x18, X_NONE )
    MASK2( "NEG",   modrm, 0xFF, 0x38, 0x18, X_NONE )
    
    MASK2( "MUL",   modrm, 0xFE, 0x38, 0x20, X_NONE )
    MASK2( "MUL",   modrm, 0xFF, 0x38, 0x20, X_NONE )
    
    MASK2( "IMUL",  modrm, 0xFE, 0x38, 0x28, X_NONE )
    MASK2( "IMUL",  modrm, 0xFF, 0x38, 0x28, X_NONE )

    MASK2( "DIV",   modrm, 0xFE, 0x38, 0x30, X_NONE )
    MASK2( "DIV",   modrm, 0xFF, 0x38, 0x30, X_NONE )
    
    MASK2( "IDIV",  modrm, 0xFE, 0x38, 0x38, X_NONE )
    MASK2( "IDIV",  modrm, 0xFF, 0x38, 0x38, X_NONE )
    
    INSN( "AAA",    none, 0x37, X_NONE )
    INSN( "BAA",    none, 0x27, X_NONE )
    INSN( "AAS",    none, 0x3F, X_NONE )
    INSN( "DAS",    none, 0x2F, X_NONE )
    MASK2( "AAM",   gobble, 0xD4, 0xFF, 0x0A, X_NONE )
    MASK2( "AAD",   gobble, 0xD5, 0xFF, 0x0A, X_NONE )
    INSN( "CBW",    none, 0x98, X_NONE )
    INSN( "CWD",    none, 0x99, X_NONE )
    
/*----------------------------------------------------------------------------
  LOGIC
  ----------------------------------------------------------------------------*/
  
    MASK2( "NOT",   modrm, 0xFE, 0x38, 0x10, X_NONE )
    MASK2( "NOT",   modrm, 0xFF, 0x38, 0x10, X_NONE )
    
#define SHIFT_ROT_GRP(M_name,M_mask) \
    MASK2( M_name, modrmC, 0xD0, 0x38, M_mask, X_NONE ) \
    MASK2( M_name, modrmC, 0xD1, 0x38, M_mask, X_NONE ) \
    MASK2( M_name, modrmC, 0xD2, 0x38, M_mask, X_NONE ) \
    MASK2( M_name, modrmC, 0xD3, 0x38, M_mask, X_NONE )
    
    SHIFT_ROT_GRP( "SHL", 0x20 )
    SHIFT_ROT_GRP( "SHR", 0x28 )
    SHIFT_ROT_GRP( "SAR", 0x38 )
    SHIFT_ROT_GRP( "ROL", 0x00 )
    SHIFT_ROT_GRP( "ROR", 0x08 )
    SHIFT_ROT_GRP( "RCL", 0x10 )
    SHIFT_ROT_GRP( "RCR", 0x18 )  

/*----------------------------------------------------------------------------
  STRING MANIPULATION
  ----------------------------------------------------------------------------*/

    PREFIX( pfx_rep, 0xF2 )
    PREFIX( pfx_rep, 0xF3 )
  
    INSN( "MOVSB", none, 0xA4, X_NONE )
    INSN( "MOVSW", none, 0xA5, X_NONE )
    
    INSN( "CMPSB", none, 0xA6, X_NONE )
    INSN( "CMPSW", none, 0xA7, X_NONE )
    
    INSN( "MOVSB", none, 0xA4, X_NONE )
    INSN( "MOVSW", none, 0xA5, X_NONE )
    
    INSN( "SCASB", none, 0xAE, X_NONE )
    INSN( "SCASW", none, 0xAF, X_NONE )
    
    INSN( "LODSB", none, 0xAC, X_NONE )
    INSN( "LODSW", none, 0xAD, X_NONE )
    
    INSN( "STOSB", none, 0xAA, X_NONE )
    INSN( "STOSW", none, 0xAB, X_NONE )
  
/*----------------------------------------------------------------------------
  CONTROL TRANSFER
  ----------------------------------------------------------------------------*/
  
    INSN( "CALL",  disp16, 0xE8, X_CALL )
    INSN( "CALL",  segoff, 0x9A, X_CALL )
    MASK2( "CALL", modrm, 0xFF, 0x38, 0x10, X_CALL )
    MASK2( "CALL", modrm, 0xFF, 0x38, 0x18, X_CALL )
  
    INSN( "JMP",   disp8,  0xEB, X_JMP )
    INSN( "JMP",   disp16, 0xE9, X_JMP )
    INSN( "JMP",   segoff, 0xEA, X_JMP )
    MASK2( "JMP",  modrm, 0xFF, 0x38, 0x20, X_JMP )
    MASK2( "JMP",  modrm, 0xFF, 0x38, 0x28, X_JMP )
  
    INSN( "RETN",  none,   0xC3, X_NONE )
    INSN( "RETN",  imm16,  0xC2, X_NONE )
    INSN( "RETF",  none,   0xCB, X_NONE )
    INSN( "RETF",  imm16,  0xCA, X_NONE )
  
    INSN( "JO",    disp8,  0x70, X_JMP )
    INSN( "JNO",   disp8,  0x71, X_JMP )
    INSN( "JB",    disp8,  0x72, X_JMP )
    INSN( "JNB",   disp8,  0x73, X_JMP )
    INSN( "JE",    disp8,  0x74, X_JMP )
    INSN( "JNE",   disp8,  0x75, X_JMP )
    INSN( "JBE",   disp8,  0x76, X_JMP )
    INSN( "JNBE",  disp8,  0x77, X_JMP )
    INSN( "JS",    disp8,  0x78, X_JMP )
    INSN( "JNS",   disp8,  0x79, X_JMP )
    INSN( "JP",    disp8,  0x7A, X_JMP )
    INSN( "JNP",   disp8,  0x7B, X_JMP )
    INSN( "JL",    disp8,  0x7C, X_JMP )
    INSN( "JNL",   disp8,  0x7D, X_JMP )
    INSN( "JLE",   disp8,  0x7E, X_JMP )
    INSN( "JNLE",  disp8,  0x7F, X_JMP )
    
    INSN( "LOOP",  disp8,  0xE2, X_JMP )
    INSN( "LOOPZ", disp8,  0xE0, X_JMP )
    INSN( "LOOPNZ",disp8,  0xE1, X_JMP )
    INSN( "JCXZ",  disp8,  0xE3, X_JMP )

    INSN( "INT",   imm8,   0xCD, X_NONE )
    INSN( "INT3",  none,   0xCC, X_NONE )
    INSN( "INTO",  none,   0xCE, X_NONE )
    INSN( "IRET",  none,   0xCF, X_NONE )

/*----------------------------------------------------------------------------
  PROCESSOR CONTROL
  ----------------------------------------------------------------------------*/
  
    INSN( "CLC",    none,  0xF8, X_NONE )
    INSN( "CMC",    none,  0xF5, X_NONE )
    INSN( "STC",    none,  0xF9, X_NONE )
    
    INSN( "CLD",    none,  0xFC, X_NONE )
    INSN( "STD",    none,  0xFD, X_NONE )
    
    INSN( "CLI",    none,  0xFA, X_NONE )
    INSN( "STI",    none,  0xFB, X_NONE )
    
    INSN( "HLT",    none,  0xF4, X_NONE )
    INSN( "WAIT",   none,  0x9B, X_NONE )
    
    MASK( "ESC",     modrm, 0xF8, 0xD8, X_NONE )
    
    INSN( "LOCK",   none,  0xF0, X_NONE )
    
/*----------------------------------------------------------------------------
  SEGMENT OVERRIDE PREFIX
  ----------------------------------------------------------------------------*/

    PREFIX( pfx_seg, 0x26 ) /* ES */
    PREFIX( pfx_seg, 0x2E ) /* CS */
    PREFIX( pfx_seg, 0x36 ) /* SS */
    PREFIX( pfx_seg, 0x3E ) /* DS */
  
/*----------------------------------------------------------------------------*/

   END
};

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
