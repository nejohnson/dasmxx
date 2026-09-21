/*****************************************************************************
 *
 * Copyright (C) 2026, Neil Johnson
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

#include "dasmxx.h"
#include "optab.h"

/*****************************************************************************
 * Globally-visible decoder properties
 *****************************************************************************/

DASM_PROFILE( "dasm430", "TI MSP430", 6, 7, 0, 2, 1 )

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/

#define FORMAT_NUM_16BIT        "0x%04X"

static const char *regs[] = {
    "PC", "SP", "SR", "CG",
    "R4", "R5", "R6", "R7",
    "R8", "R9", "R10", "R11",
    "R12", "R13", "R14", "R15"
};

static const char *dual_ops[] = {
    NULL, NULL, NULL, NULL,
    "MOV", "ADD", "ADDC", "SUBC",
    "SUB", "CMP", "DADD", "BIT",
    "BIC", "BIS", "XOR", "AND"
};

static const char *jump_ops[] = {
    "JNE", "JEQ", "JNC", "JC",
    "JN",  "JGE", "JL",  "JMP"
};

static void addr16( UWORD a, XREF_TYPE xtype )
{
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, a ) );
    xref_addxref( xtype, g_insn_addr, a );
}

static int sign_extend10( unsigned int v )
{
    v &= 0x03FF;
    if ( v & 0x0200 )
        return (int)v - 0x0400;
    return (int)v;
}

static void reg( unsigned int r )
{
    operand( "%s", regs[r & 0x0F] );
}

static UWORD next_word( FILE *f, ADDR *addr )
{
    (void)f;
    return nextw( f, addr );
}

static void indexed_addr( FILE *f, ADDR *addr, unsigned int r, XREF_TYPE xtype )
{
    UWORD disp = next_word( f, addr );

    if ( r == 0 )
    {
        ADDR target = *addr + (WORD)disp;
        addr16( target, xtype );
    }
    else if ( r == 2 )
    {
        operand( "&" );
        addr16( disp, xtype );
    }
    else
    {
        operand( FORMAT_NUM_16BIT "(%s)", disp, regs[r & 0x0F] );
    }
}

static void src_operand( FILE *f, ADDR *addr, unsigned int r, unsigned int mode,
                         XREF_TYPE xtype )
{
    if ( r == 3 )
    {
        static const char *constants[] = { "#0", "#1", "#2", "#-1" };
        operand( "%s", constants[mode & 0x03] );
        return;
    }

    if ( r == 2 )
    {
        switch ( mode & 0x03 )
        {
        case 0:
            reg( r );
            return;
        case 1:
            indexed_addr( f, addr, r, xtype );
            return;
        case 2:
            operand( "#4" );
            return;
        case 3:
            operand( "#8" );
            return;
        }
    }

    switch ( mode & 0x03 )
    {
    case 0:
        reg( r );
        break;
    case 1:
        indexed_addr( f, addr, r, xtype );
        break;
    case 2:
        operand( "@%s", regs[r & 0x0F] );
        break;
    case 3:
        if ( r == 0 )
        {
            UWORD imm = next_word( f, addr );
            if ( xtype == X_CALL || xtype == X_JMP )
            {
                operand( "#" );
                addr16( imm, xtype );
            }
            else
            {
                operand( "#" FORMAT_NUM_16BIT, imm );
            }
        }
        else
        {
            operand( "@%s+", regs[r & 0x0F] );
        }
        break;
    }
}

static void dst_operand( FILE *f, ADDR *addr, unsigned int r, unsigned int mode,
                         XREF_TYPE xtype )
{
    if ( mode == 0 )
        reg( r );
    else
        indexed_addr( f, addr, r, xtype );
}

static const char *opcode_with_size( const char *base, OPC opc )
{
    static char text[16];

    sprintf( text, "%s%s", base, ( opc & 0x0040 ) ? ".B" : "" );
    return text;
}

static const char *opcode_dual( OPC opc )
{
    return opcode_with_size( dual_ops[( opc >> 12 ) & 0x0F], opc );
}

static const char *opcode_rrc( OPC opc )  { return opcode_with_size( "RRC",  opc ); }
static const char *opcode_rra( OPC opc )  { return opcode_with_size( "RRA",  opc ); }
static const char *opcode_push( OPC opc ) { return opcode_with_size( "PUSH", opc ); }
static const char *opcode_jump( OPC opc ) { return jump_ops[( opc >> 10 ) & 0x07]; }

#define MASK_DYN(M_opcode_fn, M_ops, M_mask, M_val, M_xt) \
    { .type      = OPTAB_MASK,                            \
      .opcode    = "DYNAMIC",                             \
      .opcode_fn = opcode_ ## M_opcode_fn,                 \
      .operands  = operand_ ## M_ops,                      \
      .xtype     = M_xt,                                   \
      .u.mask.mask = M_mask,                               \
      .u.mask.val  = M_val                                 \
    },

/*****************************************************************************
 *        Private Functions
 *****************************************************************************/

OPERAND_FUNC(none)
{
    /* empty */
}

OPERAND_FUNC(single)
{
    unsigned int r = opc & 0x0F;
    unsigned int as = ( opc >> 4 ) & 0x03;

    src_operand( f, addr, r, as, xtype );
}

OPERAND_FUNC(jump)
{
    ADDR dest = *addr + ( sign_extend10( opc ) * 2 );

    addr16( dest, X_JMP );
}

OPERAND_FUNC(dual)
{
    unsigned int src = ( opc >> 8 ) & 0x0F;
    unsigned int as  = ( opc >> 4 ) & 0x03;
    unsigned int ad  = ( opc >> 7 ) & 0x01;
    unsigned int dst = opc & 0x0F;

    src_operand( f, addr, src, as, xtype );
    operand( "," );
    dst_operand( f, addr, dst, ad, xtype );
}

/*****************************************************************************
 * Instruction Decoding Tables
 *****************************************************************************/

optab_t base_optab[] = {
    /*
     * Format II: single-operand instructions.
     */
    INSN ( "RETI", none,   0x1300, X_NONE )
    MASK_DYN ( rrc,  single, 0xFF80, 0x1000, X_NONE )
    MASK     ( "SWPB", single, 0xFF80, 0x1080, X_NONE )
    MASK_DYN ( rra,  single, 0xFF80, 0x1100, X_NONE )
    MASK     ( "SXT",  single, 0xFF80, 0x1180, X_NONE )
    MASK_DYN ( push, single, 0xFF80, 0x1200, X_NONE )
    MASK     ( "CALL", single, 0xFF80, 0x1280, X_CALL )

    /*
     * Format III: PC-relative conditional and unconditional jumps.
     */
    MASK_DYN ( jump, jump, 0xE000, 0x2000, X_JMP )

    /*
     * Format I: dual-operand instructions.
     */
    MASK_DYN ( dual, dual, 0xF000, 0x4000, X_PTR )
    MASK_DYN ( dual, dual, 0xF000, 0x5000, X_PTR )
    MASK_DYN ( dual, dual, 0xF000, 0x6000, X_PTR )
    MASK_DYN ( dual, dual, 0xF000, 0x7000, X_PTR )
    MASK_DYN ( dual, dual, 0xF000, 0x8000, X_PTR )
    MASK_DYN ( dual, dual, 0xF000, 0x9000, X_PTR )
    MASK_DYN ( dual, dual, 0xF000, 0xA000, X_PTR )
    MASK_DYN ( dual, dual, 0xF000, 0xB000, X_PTR )
    MASK_DYN ( dual, dual, 0xF000, 0xC000, X_PTR )
    MASK_DYN ( dual, dual, 0xF000, 0xD000, X_PTR )
    MASK_DYN ( dual, dual, 0xF000, 0xE000, X_PTR )
    MASK_DYN ( dual, dual, 0xF000, 0xF000, X_PTR )

    END
};
