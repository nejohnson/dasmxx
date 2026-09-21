/*****************************************************************************
 *
 * Copyright (C) 2014-2015, Neil Johnson
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

DASM_PROFILE( "dasm02", "MOS Technology 6502 / WDC 65C02 / WDC 65816", 4, 9, 0, 1, 1 )

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/

/* Common output formats */
#define FORMAT_NUM_8BIT         "$%02X"
#define FORMAT_NUM_16BIT        "$%04X"
#define FORMAT_REG              "R%d"

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
    
    operand( "#" FORMAT_NUM_8BIT, byte );
}

OPERAND_FUNC(imm16)
{
    UBYTE low  = next( f, addr );
    UBYTE high = next( f, addr );
    UWORD word = MK_WORD( low, high );
    
    operand( "#" FORMAT_NUM_16BIT, word );
}

OPERAND_FUNC(imm_acc)
{
    if ( dasm_acc_width == 16 )
        operand_imm16( f, addr, opc, xtype );
    else
        operand_imm8( f, addr, opc, xtype );
}

OPERAND_FUNC(imm_idx)
{
    if ( dasm_idx_width == 16 )
        operand_imm16( f, addr, opc, xtype );
    else
        operand_imm8( f, addr, opc, xtype );
}

/***********************************************************
 * Process "zeropage" operands.
 ************************************************************/

OPERAND_FUNC(zeropage)
{
    UBYTE zp = next( f, addr );
    
    operand( xref_genwordaddr( NULL, FORMAT_NUM_8BIT, (ADDR)zp ) );
    xref_addxref( xtype, g_insn_addr, zp );
}

/***********************************************************
 * Process "zeropage,X" operands.
 ************************************************************/

OPERAND_FUNC(zeropage_X)
{
    operand_zeropage( f, addr, opc, xtype );
    COMMA;
    operand( "X" );
}

/***********************************************************
 * Process "zeropage,Y" operands.
 ************************************************************/

OPERAND_FUNC(zeropage_Y)
{
    operand_zeropage( f, addr, opc, xtype );
    COMMA;
    operand( "Y" );
}

/***********************************************************
 * Process "abs16" operand.
 ************************************************************/

OPERAND_FUNC(abs16)
{
    UBYTE low_addr  = next( f, addr );
    UBYTE high_addr = next( f, addr );
    UWORD addr16    = MK_WORD( low_addr, high_addr );

    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr16 ) );
    xref_addxref( xtype, g_insn_addr, addr16 );
}

/***********************************************************
 * Process "abs16_X" operand.
 ************************************************************/

OPERAND_FUNC(abs16_X)
{
    UBYTE low_addr  = next( f, addr );
    UBYTE high_addr = next( f, addr );
    UWORD addr16    = MK_WORD( low_addr, high_addr );

    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr16 ) );
    COMMA;
    operand( "X" );
    
    xref_addxref( xtype, g_insn_addr, addr16 );
}

/***********************************************************
 * Process "abs16_Y" operand.
 ************************************************************/

OPERAND_FUNC(abs16_Y)
{
    UBYTE low_addr  = next( f, addr );
    UBYTE high_addr = next( f, addr );
    UWORD addr16    = MK_WORD( low_addr, high_addr );

    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr16 ) );
    COMMA;
    operand( "Y" );
    
    xref_addxref( xtype, g_insn_addr, addr16 );
}

/***********************************************************
 * Process "abs24" operand.
 ************************************************************/

OPERAND_FUNC(abs24)
{
    UBYTE low_addr  = next( f, addr );
    UBYTE mid_addr  = next( f, addr );
    UBYTE high_addr = next( f, addr );
    ADDR addr24     = (ADDR)low_addr | ( (ADDR)mid_addr << 8 ) | ( (ADDR)high_addr << 16 );

    operand( xref_genwordaddr( NULL, "$%06X", addr24 ) );
    xref_addxref( xtype, g_insn_addr, addr24 );
}

/***********************************************************
 * Process "abs24_X" operand.
 ************************************************************/

OPERAND_FUNC(abs24_X)
{
    UBYTE low_addr  = next( f, addr );
    UBYTE mid_addr  = next( f, addr );
    UBYTE high_addr = next( f, addr );
    ADDR addr24     = (ADDR)low_addr | ( (ADDR)mid_addr << 8 ) | ( (ADDR)high_addr << 16 );

    operand( xref_genwordaddr( NULL, "$%06X", addr24 ) );
    COMMA;
    operand( "X" );
    
    xref_addxref( xtype, g_insn_addr, addr24 );
}

/***********************************************************
 * Process "zeropage,S" operands.
 ************************************************************/

OPERAND_FUNC(stackrel)
{
    operand_zeropage( f, addr, opc, xtype );
    COMMA;
    operand( "S" );
}

/***********************************************************
 * Process "(ind8,X)" operands.
 ************************************************************/

OPERAND_FUNC(ind8_X)
{
    operand( "(" );
    operand_zeropage( f, addr, opc, xtype );
    COMMA;
    operand( "X)" );
}

/***********************************************************
 * Process "(ind8),Y" operands.
 ************************************************************/

OPERAND_FUNC(ind8_Y)
{
    operand( "(" );
    operand_zeropage( f, addr, opc, xtype );
    operand( ")" );
    COMMA;
    operand( "Y" );
}

/***********************************************************
 * Process "(ind8,S),Y" operands.
 ************************************************************/

OPERAND_FUNC(ind8_S_Y)
{
    operand( "(" );
    operand_zeropage( f, addr, opc, xtype );
    COMMA;
    operand( "S), Y" );
}

/***********************************************************
 * Process "(ind8)" operands.
 ************************************************************/

OPERAND_FUNC(ind8)
{
    operand( "(" );
    operand_zeropage( f, addr, opc, xtype );
    operand( ")" );
}

/***********************************************************
 * Process "[ind8]" operands.
 ************************************************************/

OPERAND_FUNC(ind8_long)
{
    operand( "[" );
    operand_zeropage( f, addr, opc, xtype );
    operand( "]" );
}

/***********************************************************
 * Process "[ind8],Y" operands.
 ************************************************************/

OPERAND_FUNC(ind8_long_Y)
{
    operand( "[" );
    operand_zeropage( f, addr, opc, xtype );
    operand( "]" );
    COMMA;
    operand( "Y" );
}

/***********************************************************
 * Process "ind16" operand.
 ************************************************************/

OPERAND_FUNC(ind16)
{
    UBYTE low_addr  = next( f, addr );
    UBYTE high_addr = next( f, addr );
    UWORD addr16    = MK_WORD( low_addr, high_addr );

    operand( "(%s)", xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr16 ) );
    
    xref_addxref( xtype, g_insn_addr, addr16 );
}

/***********************************************************
 * Process "(abs16,X)" operand.
 ************************************************************/

OPERAND_FUNC(ind16_X)
{
    UBYTE low_addr  = next( f, addr );
    UBYTE high_addr = next( f, addr );
    UWORD addr16    = MK_WORD( low_addr, high_addr );

    operand( "(%s", xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr16 ) );
    COMMA;
    operand( "X)" );
    
    xref_addxref( xtype, g_insn_addr, addr16 );
}

/***********************************************************
 * Process "[abs16]" operand.
 ************************************************************/

OPERAND_FUNC(ind16_long)
{
    UBYTE low_addr  = next( f, addr );
    UBYTE high_addr = next( f, addr );
    UWORD addr16    = MK_WORD( low_addr, high_addr );

    operand( "[%s]", xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr16 ) );
    
    xref_addxref( xtype, g_insn_addr, addr16 );
}

/***********************************************************
 * Process "rel8" operands.
 ************************************************************/

OPERAND_FUNC(rel8)
{
    BYTE disp = (BYTE)next( f, addr );
    ADDR dest = *addr + disp;
    
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

/***********************************************************
 * Process "rel16" operands.
 ************************************************************/

OPERAND_FUNC(rel16)
{
    UBYTE low  = next( f, addr );
    UBYTE high = next( f, addr );
    WORD disp = (WORD)MK_WORD( low, high );
    ADDR dest = *addr + disp;
    
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

/***********************************************************
 * Process two 8-bit immediate operands.
 ************************************************************/

OPERAND_FUNC(imm8_imm8)
{
    UBYTE left  = next( f, addr );
    UBYTE right = next( f, addr );
    
    operand( FORMAT_NUM_8BIT, left );
    COMMA;
    operand( FORMAT_NUM_8BIT, right );
}

/***********************************************************
 * Process "zeropage, rel8" bit-branch operands.
 ************************************************************/

OPERAND_FUNC(zeropage_rel8)
{
    UBYTE zp = next( f, addr );
    BYTE disp = (BYTE)next( f, addr );
    ADDR dest = *addr + disp;
    
    operand( "%s", xref_genwordaddr( NULL, FORMAT_NUM_8BIT, (ADDR)zp ) );
    COMMA;
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( X_PTR, g_insn_addr, zp );
    xref_addxref( xtype, g_insn_addr, dest );
}

/******************************************************************************/
/**                            Dynamic Opcodes                               **/
/******************************************************************************/

static const char *opcode_rmb( OPC opc )
{
    static char name[] = "rmb0";
    name[3] = '0' + (char)( ( opc >> 4 ) & 0x07 );
    return name;
}

static const char *opcode_smb( OPC opc )
{
    static char name[] = "smb0";
    name[3] = '0' + (char)( ( opc >> 4 ) & 0x07 );
    return name;
}

static const char *opcode_bbr( OPC opc )
{
    static char name[] = "bbr0";
    name[3] = '0' + (char)( ( opc >> 4 ) & 0x07 );
    return name;
}

static const char *opcode_bbs( OPC opc )
{
    static char name[] = "bbs0";
    name[3] = '0' + (char)( ( opc >> 4 ) & 0x07 );
    return name;
}

/******************************************************************************/
/** Instruction Decoding Tables                                              **/
/** Note: tables are here as they refer to operand functions defined above.  **/
/******************************************************************************/

optab_t base_optab[] = {

#undef ACC_OP
#define ACC_OP(M_name, M_base) \
    INSN ( M_name, imm_acc,    ( 0x09 | M_base ), X_NONE ) \
    INSN ( M_name, zeropage,   ( 0x05 | M_base ), X_PTR  ) \
    INSN ( M_name, zeropage_X, ( 0x15 | M_base ), X_PTR  ) \
    INSN ( M_name, abs16,      ( 0x0D | M_base ), X_PTR  ) \
    INSN ( M_name, abs16_X,    ( 0x1D | M_base ), X_PTR  ) \
    INSN ( M_name, abs16_Y,    ( 0x19 | M_base ), X_PTR  ) \
    INSN ( M_name, ind8_X,     ( 0x01 | M_base ), X_PTR  ) \
    INSN ( M_name, ind8_Y,     ( 0x11 | M_base ), X_PTR  ) \
    INSN_CPU ( M_name, stackrel,    ( 0x03 | M_base ), X_PTR, CPU_65816 ) \
    INSN_CPU ( M_name, ind8_S_Y,    ( 0x13 | M_base ), X_PTR, CPU_65816 ) \
    INSN_CPU ( M_name, ind8_long,   ( 0x07 | M_base ), X_PTR, CPU_65816 ) \
    INSN_CPU ( M_name, ind8_long_Y, ( 0x17 | M_base ), X_PTR, CPU_65816 ) \
    INSN_CPU ( M_name, abs24,       ( 0x0F | M_base ), X_PTR, CPU_65816 ) \
    INSN_CPU ( M_name, abs24_X,     ( 0x1F | M_base ), X_PTR, CPU_65816 )

/*----------------------------------------------------------------------------
  Load/Store
  ----------------------------------------------------------------------------*/

    ACC_OP( "lda", 0xA0 )
    INSN_CPU ( "lda", ind8,   0xB2, X_PTR, CPU_65C02 )
    
    INSN ( "ldx", imm_idx,    0xA2, X_NONE )
    INSN ( "ldx", zeropage,   0xA6, X_PTR  )
    INSN ( "ldx", zeropage_Y, 0xB6, X_PTR  )
    INSN ( "ldx", abs16,      0xAE, X_PTR  )
    INSN ( "ldx", abs16_Y,    0xBE, X_PTR  )
    
    INSN ( "ldy", imm_idx,    0xA0, X_NONE )
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
    INSN_CPU ( "sta", ind8,   0x92, X_PTR, CPU_65C02 )
    
    INSN ( "stx", zeropage,   0x86, X_PTR  )
    INSN ( "stx", zeropage_Y, 0x96, X_PTR  )
    INSN ( "stx", abs16,      0x8E, X_PTR  )
    
    INSN ( "sty", zeropage,   0x84, X_PTR  )
    INSN ( "sty", zeropage_X, 0x94, X_PTR  )
    INSN ( "sty", abs16,      0x8C, X_PTR  )

    INSN_CPU ( "stz", zeropage,   0x64, X_PTR, CPU_65C02 )
    INSN_CPU ( "stz", zeropage_X, 0x74, X_PTR, CPU_65C02 )
    INSN_CPU ( "stz", abs16,      0x9C, X_PTR, CPU_65C02 )
    INSN_CPU ( "stz", abs16_X,    0x9E, X_PTR, CPU_65C02 )
    
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
    INSN_CPU ( "phx",  none,  0xDA, X_NONE, CPU_65C02 )
    INSN_CPU ( "phy",  none,  0x5A, X_NONE, CPU_65C02 )
    INSN_CPU ( "plx",  none,  0xFA, X_NONE, CPU_65C02 )
    INSN_CPU ( "ply",  none,  0x7A, X_NONE, CPU_65C02 )
    
/*----------------------------------------------------------------------------
  Logical
  ----------------------------------------------------------------------------*/

    ACC_OP( "and", 0x20 )
    ACC_OP( "eor", 0x40 )
    ACC_OP( "ora", 0x00 )
    INSN_CPU ( "and", ind8,   0x32, X_PTR, CPU_65C02 )
    INSN_CPU ( "eor", ind8,   0x52, X_PTR, CPU_65C02 )
    INSN_CPU ( "ora", ind8,   0x12, X_PTR, CPU_65C02 )
    
    INSN ( "bit", zeropage,   0x24, X_PTR  )
    INSN ( "bit", abs16,      0x2C, X_PTR  )
    INSN_CPU ( "bit", imm8,       0x89, X_NONE, CPU_65C02 )
    INSN_CPU ( "bit", zeropage_X, 0x34, X_PTR,  CPU_65C02 )
    INSN_CPU ( "bit", abs16_X,    0x3C, X_PTR,  CPU_65C02 )

    INSN_CPU ( "trb", zeropage,   0x14, X_PTR, CPU_65C02 )
    INSN_CPU ( "trb", abs16,      0x1C, X_PTR, CPU_65C02 )
    INSN_CPU ( "tsb", zeropage,   0x04, X_PTR, CPU_65C02 )
    INSN_CPU ( "tsb", abs16,      0x0C, X_PTR, CPU_65C02 )
    
/*----------------------------------------------------------------------------
  Arithmetic
  ----------------------------------------------------------------------------*/
 
    ACC_OP( "adc", 0x60 )
    ACC_OP( "sbc", 0xE0 )
    ACC_OP( "cmp", 0xC0 )
    INSN_CPU ( "adc", ind8,   0x72, X_PTR, CPU_65C02 )
    INSN_CPU ( "sbc", ind8,   0xF2, X_PTR, CPU_65C02 )
    INSN_CPU ( "cmp", ind8,   0xD2, X_PTR, CPU_65C02 )
    
    INSN ( "cpx", imm_idx,    0xE0, X_NONE )
    INSN ( "cpx", zeropage,   0xE4, X_PTR  )
    INSN ( "cpx", abs16,      0xEC, X_PTR  )
    
    INSN ( "cpy", imm_idx,    0xC0, X_NONE )
    INSN ( "cpy", zeropage,   0xC4, X_PTR  )
    INSN ( "cpy", abs16,      0xCC, X_PTR  )
    
/*----------------------------------------------------------------------------
  Increment/Decrement
  ----------------------------------------------------------------------------*/    
    
    INSN ( "inc", zeropage,   0xE6, X_PTR  )
    INSN ( "inc", zeropage_X, 0xF6, X_PTR  )
    INSN ( "inc", abs16,      0xEE, X_PTR  )
    INSN ( "inc", abs16_X,    0xFE, X_PTR  )
    INSN_CPU ( "inc", none,   0x1A, X_NONE, CPU_65C02 )
    
    INSN ( "inx", none,       0xE8, X_NONE )
    INSN ( "iny", none,       0xC8, X_NONE )
    
    INSN ( "dec", zeropage,   0xC6, X_PTR  )
    INSN ( "dec", zeropage_X, 0xD6, X_PTR  )
    INSN ( "dec", abs16,      0xCE, X_PTR  )
    INSN ( "dec", abs16_X,    0xDE, X_PTR  )
    INSN_CPU ( "dec", none,   0x3A, X_NONE, CPU_65C02 )
    
    INSN ( "dex", none,       0xCA, X_NONE )
    INSN ( "dey", none,       0x88, X_NONE )
    
/*----------------------------------------------------------------------------
  Shift and Rotate
  ----------------------------------------------------------------------------*/    
    
#undef ACC_OP
#define ACC_OP(M_name, M_base)        \
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
    INSN_CPU ( "jmp", ind16_X,0x7C, X_PTR, CPU_65C02 )
    INSN_CPU ( "jml", abs24,  0x5C, X_JMP, CPU_65816 )
    INSN_CPU ( "jml", ind16_long, 0xDC, X_PTR, CPU_65816 )
    INSN ( "jsr", abs16,      0x20, X_CALL )
    INSN_CPU ( "jsl", abs24,  0x22, X_CALL, CPU_65816 )
    INSN_CPU ( "jsr", ind16_X,0xFC, X_PTR, CPU_65816 )
    INSN ( "rts", none,       0x60, X_NONE )
    INSN_CPU ( "rtl", none,   0x6B, X_NONE, CPU_65816 )
    
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
    INSN_CPU ( "bra", rel8, 0x80, X_JMP, CPU_65C02 )
    INSN_CPU ( "brl", rel16, 0x82, X_JMP, CPU_65816 )

    INSN_DYN_CPU_RANGE ( rmb, zeropage,      0x07, X_PTR, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( rmb, zeropage,      0x17, X_PTR, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( rmb, zeropage,      0x27, X_PTR, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( rmb, zeropage,      0x37, X_PTR, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( rmb, zeropage,      0x47, X_PTR, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( rmb, zeropage,      0x57, X_PTR, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( rmb, zeropage,      0x67, X_PTR, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( rmb, zeropage,      0x77, X_PTR, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( smb, zeropage,      0x87, X_PTR, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( smb, zeropage,      0x97, X_PTR, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( smb, zeropage,      0xA7, X_PTR, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( smb, zeropage,      0xB7, X_PTR, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( smb, zeropage,      0xC7, X_PTR, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( smb, zeropage,      0xD7, X_PTR, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( smb, zeropage,      0xE7, X_PTR, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( smb, zeropage,      0xF7, X_PTR, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( bbr, zeropage_rel8, 0x0F, X_JMP, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( bbr, zeropage_rel8, 0x1F, X_JMP, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( bbr, zeropage_rel8, 0x2F, X_JMP, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( bbr, zeropage_rel8, 0x3F, X_JMP, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( bbr, zeropage_rel8, 0x4F, X_JMP, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( bbr, zeropage_rel8, 0x5F, X_JMP, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( bbr, zeropage_rel8, 0x6F, X_JMP, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( bbr, zeropage_rel8, 0x7F, X_JMP, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( bbs, zeropage_rel8, 0x8F, X_JMP, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( bbs, zeropage_rel8, 0x9F, X_JMP, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( bbs, zeropage_rel8, 0xAF, X_JMP, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( bbs, zeropage_rel8, 0xBF, X_JMP, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( bbs, zeropage_rel8, 0xCF, X_JMP, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( bbs, zeropage_rel8, 0xDF, X_JMP, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( bbs, zeropage_rel8, 0xEF, X_JMP, CPU_65C02, CPU_65C02 )
    INSN_DYN_CPU_RANGE ( bbs, zeropage_rel8, 0xFF, X_JMP, CPU_65C02, CPU_65C02 )
    
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
    INSN_CPU ( "wai", none, 0xCB, X_NONE, CPU_65C02 )
    INSN_CPU ( "stp", none, 0xDB, X_NONE, CPU_65C02 )
    INSN_CPU ( "cop", imm8, 0x02, X_NONE, CPU_65816 )
    INSN_CPU ( "mvn", imm8_imm8, 0x54, X_NONE, CPU_65816 )
    INSN_CPU ( "mvp", imm8_imm8, 0x44, X_NONE, CPU_65816 )
    INSN_CPU ( "pea", imm16, 0xF4, X_IMM, CPU_65816 )
    INSN_CPU ( "pei", ind8, 0xD4, X_PTR, CPU_65816 )
    INSN_CPU ( "per", rel16, 0x62, X_IMM, CPU_65816 )
    INSN_CPU ( "phb", none, 0x8B, X_NONE, CPU_65816 )
    INSN_CPU ( "phd", none, 0x0B, X_NONE, CPU_65816 )
    INSN_CPU ( "phk", none, 0x4B, X_NONE, CPU_65816 )
    INSN_CPU ( "plb", none, 0xAB, X_NONE, CPU_65816 )
    INSN_CPU ( "pld", none, 0x2B, X_NONE, CPU_65816 )
    INSN_CPU ( "rep", imm8, 0xC2, X_NONE, CPU_65816 )
    INSN_CPU ( "sep", imm8, 0xE2, X_NONE, CPU_65816 )
    INSN_CPU ( "tcd", none, 0x5B, X_NONE, CPU_65816 )
    INSN_CPU ( "tcs", none, 0x1B, X_NONE, CPU_65816 )
    INSN_CPU ( "tdc", none, 0x7B, X_NONE, CPU_65816 )
    INSN_CPU ( "tsc", none, 0x3B, X_NONE, CPU_65816 )
    INSN_CPU ( "txy", none, 0x9B, X_NONE, CPU_65816 )
    INSN_CPU ( "tyx", none, 0xBB, X_NONE, CPU_65816 )
    INSN_CPU ( "xba", none, 0xEB, X_NONE, CPU_65816 )
    INSN_CPU ( "xce", none, 0xFB, X_NONE, CPU_65816 )

    END
};

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
