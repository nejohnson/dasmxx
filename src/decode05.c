/*****************************************************************************
 *
 * Copyright (C) 2018, Gareth Edwards
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

DASM_PROFILE( "dasm05", "Motorola 6805", 3, 9, 1, 1 )

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/

/* Common output formats */
#define FORMAT_NUM_8BIT         "$%02X"
#define FORMAT_NUM_16BIT        "$%04X"

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

/***********************************************************
 * Process "#imm8" operands.
 *    byte comes from next byte.
 ************************************************************/

OPERAND_FUNC(imm8)
{
    UBYTE byte = next( f, addr );
    
    operand( "#" FORMAT_NUM_8BIT, byte );
}


OPERAND_FUNC(direct)
{
    UBYTE a = next( f, addr );

    operand( xref_genwordaddr( NULL, FORMAT_NUM_8BIT, (ADDR)a ));
    xref_addxref( xtype, g_insn_addr, a);
}

OPERAND_FUNC(rel8)
{
    BYTE disp = (BYTE)next( f, addr );
    ADDR dest = *addr + disp;

    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

OPERAND_FUNC(extended)
{
    UBYTE msb    = next( f, addr );
    UBYTE lsb    = next( f, addr );
    UWORD addr16 = MK_WORD( lsb, msb );

    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr16 ) );
    xref_addxref( xtype, g_insn_addr, addr16 );
}

OPERAND_FUNC(bitmanip)
{
    operand( "%1d", (opc >> 1) & 0x7 );
}

OPERAND_FUNC(btb)
{
    operand_bitmanip( f, addr, opc, xtype );
    COMMA;
    operand_direct( f, addr, opc, X_DIRECT );
    COMMA;
    operand_rel8( f, addr, opc, xtype );
}

OPERAND_FUNC(bsc)
{
    operand_bitmanip( f, addr, opc, xtype );
    COMMA;
    operand_direct( f, addr, opc, xtype );
}

OPERAND_FUNC(ix)
{
    operand( ",x" );
}

OPERAND_FUNC(ix1)
{
    operand_direct( f, addr, opc, xtype );
    operand_ix( f, addr, opc, xtype );
}

OPERAND_FUNC(ix2)
{
    operand_extended( f, addr, opc, xtype );
    operand_ix( f, addr, opc, xtype );
}

/******************************************************************************/
/** Instruction Decoding Tables                                              **/
/** Note: tables are here as they refer to operand functions defined above.  **/
/******************************************************************************/

optab_t base_optab[] = {
    /*
    * Bit Manipulation
    */
    /*
    * BTB - Bit test and branch
    */
    MASK ( "brset", btb, 0xF1, 0x00, X_JMP )
    MASK ( "brclr", btb, 0xF1, 0x01, X_JMP )
    /* 
    * BSC - Bit set/clear
    */
    MASK ( "bset", bsc, 0xF1, 0x10, X_DIRECT )
    MASK ( "bclr", bsc, 0xF1, 0x11, X_DIRECT )

    /*
    * Branches
    */
    INSN ( "bra",  rel8, 0x20, X_JMP )
    INSN ( "brn",  rel8, 0x21, X_JMP )
    INSN ( "bhi",  rel8, 0x22, X_JMP )
    INSN ( "bls",  rel8, 0x23, X_JMP )
    INSN ( "bcc",  rel8, 0x24, X_JMP )
    INSN ( "bcs",  rel8, 0x25, X_JMP )
    INSN ( "bne",  rel8, 0x26, X_JMP )
    INSN ( "beq",  rel8, 0x27, X_JMP )
    INSN ( "bhcc", rel8, 0x28, X_JMP )
    INSN ( "bhcs", rel8, 0x29, X_JMP )
    INSN ( "bpl",  rel8, 0x2A, X_JMP )
    INSN ( "bmi",  rel8, 0x2B, X_JMP )
    INSN ( "bmc",  rel8, 0x2C, X_JMP )
    INSN ( "bms",  rel8, 0x2D, X_JMP )
    INSN ( "bil",  rel8, 0x2E, X_JMP )
    INSN ( "bih",  rel8, 0x2F, X_JMP )

    /* 
    * RMW
    */
#define RMW_OP(M_name, M_base) \
        INSN ( M_name,     direct, ( 0x30 | M_base ), X_DIRECT ) \
        INSN ( M_name "a", none,   ( 0x40 | M_base),  X_NONE ) \
        INSN ( M_name "x", none,   ( 0x50 | M_base),  X_NONE ) \
        INSN ( M_name,     ix1,    (0x60 | M_base),   X_PTR ) \
        INSN ( M_name,     ix,     (0x70 | M_base),   X_DIRECT )

    INSN ( "neg", none, 0x40, X_NONE )
    INSN ( "neg", none, 0x50, X_NONE )

    RMW_OP( "neg", 0x00 )
    RMW_OP( "com", 0x03 )
    RMW_OP( "lsr", 0x04 )
    RMW_OP( "ror", 0x06 )
    RMW_OP( "asr", 0x07 )
    RMW_OP( "lsl", 0x08 )
    RMW_OP( "rol", 0x09 )
    RMW_OP( "dec", 0x0A )
    RMW_OP( "inc", 0x0C )
    RMW_OP( "tst", 0x0D )
    RMW_OP( "clr", 0x0F )

#define REGMEM_OP(M_name, M_base) \
        INSN ( M_name, imm8,     (0xA0 | M_base), X_NONE ) \
        INSN ( M_name, direct,   (0xB0 | M_base), X_DIRECT ) \
        INSN ( M_name, extended, (0xC0 | M_base), X_PTR ) \
        INSN ( M_name, ix2,      (0xD0 | M_base), X_PTR ) \
        INSN ( M_name, ix1,      (0xE0 | M_base), X_PTR ) \
        INSN ( M_name, ix,       (0xF0 | M_base), X_PTR)


    UNDEF( 0xA7 )
    UNDEF( 0xAC )
    UNDEF( 0xAF )

    INSN ( "bsr", rel8, 0xAD, X_CALL )

    REGMEM_OP( "sub", 0x00 )
    REGMEM_OP( "cmp", 0x01 )
    REGMEM_OP( "sbc", 0x02 )
    REGMEM_OP( "cpx", 0x03 )
    REGMEM_OP( "and", 0x04 )
    REGMEM_OP( "bit", 0x05 )
    REGMEM_OP( "lda", 0x06 )
    REGMEM_OP( "sta", 0x07 )
    REGMEM_OP( "eor", 0x08 )
    REGMEM_OP( "adc", 0x09 )
    REGMEM_OP( "ora", 0x0A )
    REGMEM_OP( "add", 0x0B )
    REGMEM_OP( "jmp", 0x0C )
    REGMEM_OP( "jsr", 0x0D )
    REGMEM_OP( "ldx", 0x0E )
    REGMEM_OP( "stx", 0x0F )

    /*
    * Control
    */
    INSN ( "rti",  none, 0x80, X_NONE )
    INSN ( "rts",  none, 0x81, X_NONE )
    INSN ( "swi",  none, 0x83, X_NONE )
    INSN ( "stop", none, 0x8E, X_NONE )
    INSN ( "wait", none, 0x8F, X_NONE )

    INSN ( "tax",  none, 0x97, X_NONE )
    INSN ( "clc",  none, 0x98, X_NONE )
    INSN ( "sec",  none, 0x99, X_NONE )
    INSN ( "cli",  none, 0x9A, X_NONE )
    INSN ( "sei",  none, 0x9B, X_NONE )
    INSN ( "rsp",  none, 0x9C, X_NONE )
    INSN ( "nop",  none, 0x9D, X_NONE )
    INSN ( "txa",  none, 0x9F, X_NONE )

    END
};