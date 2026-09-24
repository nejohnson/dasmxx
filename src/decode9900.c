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

DASM_PROFILE( "dasm9900", "TI TMS9900/TMS9995", 6, 5, 1, 2, 1 )

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/

#define FORMAT_NUM_8BIT         ">%02X"
#define FORMAT_NUM_16BIT        ">%04X"

#define MK_WORD(l,h)            ( ((l) & 0xFF) | (((h) & 0xFF) << 8) )

static void reg( unsigned int r )
{
    operand( "R%u", r & 0x0F );
}

static void addr16( UWORD a, XREF_TYPE xtype )
{
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, a ) );
    xref_addxref( xtype, g_insn_addr, a );
}

int dasm_cfg_supported( void )
{
    return 1;
}

static int read_opcode_word( ADDR addr, UWORD *out )
{
    return dasm_input_read_word_at( addr, out );
}

void dasm_post_insn( void )
{
    UWORD op0;

    if ( !read_opcode_word( g_insn_addr, &op0 ) )
        return;

    if ( op0 == 0x0340 )
        dasm_cfg_set_flow( CFG_FLOW_HALT );
    else if ( op0 == 0x0380 )
        dasm_cfg_set_flow( CFG_FLOW_RETURN );
    else if ( (op0 & 0xFFC0) == 0x0400 || (op0 & 0xFC00) == 0x2C00 )
        dasm_cfg_set_flow( CFG_FLOW_INDIRECT_CALL );
    else if ( (op0 & 0xFFC0) == 0x0440 )
        dasm_cfg_set_flow( CFG_FLOW_JUMP );
    else if ( (op0 & 0xFFC0) == 0x0680 )
        dasm_cfg_set_flow( CFG_FLOW_CALL );
    else if ( (op0 & 0xFF00) == 0x1000 )
        dasm_cfg_set_flow( CFG_FLOW_JUMP );
    else if ( (op0 & 0xF000) == 0x1000 )
        dasm_cfg_set_flow( CFG_FLOW_COND_JUMP );
}

static UWORD next_word( FILE *f, ADDR *addr )
{
    (void)f;
    return nextw( f, addr );
}

static void ea( FILE *f, ADDR *addr, unsigned int spec, XREF_TYPE xtype )
{
    unsigned int mode = ( spec >> 4 ) & 0x03;
    unsigned int r = spec & 0x0F;

    switch ( mode )
    {
    case 0:
        reg( r );
        break;

    case 1:
        operand( "*R%u", r );
        break;

    case 2:
        {
            UWORD a = next_word( f, addr );

            operand( "@" );
            addr16( a, xtype );
            if ( r != 0 )
                operand( "(R%u)", r );
        }
        break;

    case 3:
        operand( "*R%u+", r );
        break;
    }
}

/*****************************************************************************
 *        Private Functions
 *****************************************************************************/

OPERAND_FUNC(none)
{
    /* empty */
}

OPERAND_FUNC(reg)
{
    reg( opc & 0x0F );
}

OPERAND_FUNC(reg_imm16)
{
    UWORD imm = next_word( f, addr );

    reg( opc & 0x0F );
    operand( ",#" FORMAT_NUM_16BIT, imm );
}

OPERAND_FUNC(imm16)
{
    UWORD imm = next_word( f, addr );

    operand( "#" FORMAT_NUM_16BIT, imm );
}

OPERAND_FUNC(shift)
{
    unsigned int count = ( opc >> 4 ) & 0x0F;

    reg( opc & 0x0F );
    operand( "," );
    if ( count == 0 )
        reg( 0 );
    else
        operand( "%u", count );
}

OPERAND_FUNC(rel8)
{
    BYTE disp = (BYTE)( opc & 0xFF );
    ADDR dest = *addr + ( disp * 2 );

    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( X_JMP, g_insn_addr, dest );
}

OPERAND_FUNC(cru8)
{
    operand( FORMAT_NUM_8BIT, opc & 0xFF );
}

OPERAND_FUNC(single)
{
    ea( f, addr, opc & 0x3F, xtype );
}

OPERAND_FUNC(src_reg)
{
    ea( f, addr, opc & 0x3F, xtype );
    operand( "," );
    reg( ( opc >> 6 ) & 0x0F );
}

OPERAND_FUNC(src_count)
{
    unsigned int count = ( opc >> 6 ) & 0x0F;

    ea( f, addr, opc & 0x3F, xtype );
    operand( ",%u", count == 0 ? 16 : count );
}

OPERAND_FUNC(twoop)
{
    unsigned int src = opc & 0x3F;
    unsigned int dst = ( opc >> 6 ) & 0x3F;

    ea( f, addr, src, xtype );
    operand( "," );
    ea( f, addr, dst, xtype );
}

/*****************************************************************************
 * Instruction Decoding Tables
 *****************************************************************************/

optab_t base_optab[] = {
    /*
     * Immediate, workspace, and status/control instructions.
     */
    MASK ( "LI",   reg_imm16, 0xFFE0, 0x0200, X_IMM )
    MASK ( "AI",   reg_imm16, 0xFFE0, 0x0220, X_IMM )
    MASK ( "ANDI", reg_imm16, 0xFFE0, 0x0240, X_IMM )
    MASK ( "ORI",  reg_imm16, 0xFFE0, 0x0260, X_IMM )
    MASK ( "CI",   reg_imm16, 0xFFE0, 0x0280, X_IMM )
    MASK ( "STWP", reg,       0xFFE0, 0x02A0, X_REG )
    MASK ( "STST", reg,       0xFFE0, 0x02C0, X_REG )
    INSN ( "LWPI", imm16, 0x02E0, X_IMM )
    INSN ( "LIMI", imm16, 0x0300, X_IMM )

    INSN ( "IDLE", none, 0x0340, X_NONE )
    INSN ( "RSET", none, 0x0360, X_NONE )
    INSN ( "RTWP", none, 0x0380, X_NONE )
    INSN ( "CKON", none, 0x03A0, X_NONE )
    INSN ( "CKOF", none, 0x03C0, X_NONE )
    INSN ( "LREX", none, 0x03E0, X_NONE )

    /*
     * Single-address instructions.
     */
    MASK ( "BLWP", single, 0xFFC0, 0x0400, X_PTR )
    MASK ( "B",    single, 0xFFC0, 0x0440, X_JMP )
    MASK ( "X",    single, 0xFFC0, 0x0480, X_PTR )
    MASK ( "CLR",  single, 0xFFC0, 0x04C0, X_PTR )
    MASK ( "NEG",  single, 0xFFC0, 0x0500, X_PTR )
    MASK ( "INV",  single, 0xFFC0, 0x0540, X_PTR )
    MASK ( "INC",  single, 0xFFC0, 0x0580, X_PTR )
    MASK ( "INCT", single, 0xFFC0, 0x05C0, X_PTR )
    MASK ( "DEC",  single, 0xFFC0, 0x0600, X_PTR )
    MASK ( "DECT", single, 0xFFC0, 0x0640, X_PTR )
    MASK ( "BL",   single, 0xFFC0, 0x0680, X_CALL )
    MASK ( "SWPB", single, 0xFFC0, 0x06C0, X_PTR )
    MASK ( "SETO", single, 0xFFC0, 0x0700, X_PTR )
    MASK ( "ABS",  single, 0xFFC0, 0x0740, X_PTR )

    /*
     * Shift instructions.
     */
    MASK ( "SRA", shift, 0xFF00, 0x0800, X_REG )
    MASK ( "SRL", shift, 0xFF00, 0x0900, X_REG )
    MASK ( "SLA", shift, 0xFF00, 0x0A00, X_REG )
    MASK ( "SRC", shift, 0xFF00, 0x0B00, X_REG )

    /*
     * Jumps and single-bit CRU instructions.
     */
#define REL_OP(M_name, M_base) MASK ( M_name, rel8, 0xFF00, M_base, X_JMP )
    REL_OP( "JMP", 0x1000 )
    REL_OP( "JLT", 0x1100 )
    REL_OP( "JLE", 0x1200 )
    REL_OP( "JEQ", 0x1300 )
    REL_OP( "JHE", 0x1400 )
    REL_OP( "JGT", 0x1500 )
    REL_OP( "JNE", 0x1600 )
    REL_OP( "JNC", 0x1700 )
    REL_OP( "JOC", 0x1800 )
    REL_OP( "JNO", 0x1900 )
    REL_OP( "JL",  0x1A00 )
    REL_OP( "JH",  0x1B00 )
    REL_OP( "JOP", 0x1C00 )
    MASK ( "SBO", cru8, 0xFF00, 0x1D00, X_IO )
    MASK ( "SBZ", cru8, 0xFF00, 0x1E00, X_IO )
    MASK ( "TB",  cru8, 0xFF00, 0x1F00, X_IO )

    /*
     * Register/source, CRU multi-bit, multiply/divide, and XOP.
     */
    MASK ( "COC",  src_reg,   0xFC00, 0x2000, X_REG )
    MASK ( "CZC",  src_reg,   0xFC00, 0x2400, X_REG )
    MASK ( "XOR",  src_reg,   0xFC00, 0x2800, X_REG )
    MASK ( "XOP",  src_reg,   0xFC00, 0x2C00, X_CALL )
    MASK ( "LDCR", src_count, 0xFC00, 0x3000, X_IO )
    MASK ( "STCR", src_count, 0xFC00, 0x3400, X_IO )
    MASK ( "MPY",  src_reg,   0xFC00, 0x3800, X_REG )
    MASK ( "DIV",  src_reg,   0xFC00, 0x3C00, X_REG )

    /*
     * Two-address instructions.
     */
    MASK ( "SZC",  twoop, 0xF000, 0x4000, X_PTR )
    MASK ( "SZCB", twoop, 0xF000, 0x5000, X_PTR )
    MASK ( "S",    twoop, 0xF000, 0x6000, X_PTR )
    MASK ( "SB",   twoop, 0xF000, 0x7000, X_PTR )
    MASK ( "C",    twoop, 0xF000, 0x8000, X_PTR )
    MASK ( "CB",   twoop, 0xF000, 0x9000, X_PTR )
    MASK ( "A",    twoop, 0xF000, 0xA000, X_PTR )
    MASK ( "AB",   twoop, 0xF000, 0xB000, X_PTR )
    MASK ( "MOV",  twoop, 0xF000, 0xC000, X_PTR )
    MASK ( "MOVB", twoop, 0xF000, 0xD000, X_PTR )
    MASK ( "SOC",  twoop, 0xF000, 0xE000, X_PTR )
    MASK ( "SOCB", twoop, 0xF000, 0xF000, X_PTR )

    END
};
