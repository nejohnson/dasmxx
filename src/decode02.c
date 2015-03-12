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

/* Decoder short name */
const char * dasm_name            = "dasm02";

/* Decoder description */
const char * dasm_description     = "MOS Technology 6502";

/* Decoder maximum instruction length in bytes */
const int    dasm_max_insn_length = 3;

/* Decoder maximum opcode field width */
const int    dasm_max_opcode_width = 9;

/* LSB at lowest address */
const int    dasm_word_msb_first   = 0;

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
 * Process "rel8" operands.
 ************************************************************/

OPERAND_FUNC(rel8)
{
    BYTE disp = (BYTE)next( f, addr );
    ADDR dest = *addr + disp;
    
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

/******************************************************************************/
/** Instruction Decoding Tables                                              **/
/** Note: tables are here as they refer to operand functions defined above.  **/
/******************************************************************************/

optab_t base_optab[] = {

#undef ACC_OP
#define ACC_OP(M_name, M_base) \
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

    END
};

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
