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

DASM_PROFILE( "dasmz8", "Zilog Z8", 4, 6, 1, 1, 1 )

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/

#define FORMAT_NUM_8BIT         "$%02X"
#define FORMAT_NUM_16BIT        "$%04X"

#define MK_WORD(l,h)            ( ((l) & 0xFF) | (((h) & 0xFF) << 8) )

#define Z8_CC_F                 0x0
#define Z8_CC_T                 0x8

static const char *alu_ops[16] = {
    "add", "adc", "sub", "sbc", "or", "and", "tcm", "tm",
    NULL,  NULL,  "cp",  "xor", NULL, NULL,  NULL,  NULL
};

static const char *cc_names[16] = {
    "f", "lt", "le", "ule", "ov", "mi", "z", "c",
    "t", "ge", "gt", "ugt", "nov", "pl", "nz", "nc"
};

static const char *ext_ops[16] = {
    "stop", "halt", "decw", NULL, "da", "pop", "com", "push",
    "decw", "rl",   "incw", "clr", "rrc", "sra", "rr",  "swap"
};

int dasm_cfg_supported( void )
{
    return 1;
}

static int read_opcode_byte( ADDR addr, UBYTE *out )
{
    return dasm_input_read_byte_at( addr, out );
}

static void z8_cfg_set_cond_jump_flow( UBYTE cc )
{
    if ( cc == Z8_CC_F )
        return;

    if ( cc == Z8_CC_T )
        dasm_cfg_set_flow( CFG_FLOW_JUMP );
    else
        dasm_cfg_set_flow( CFG_FLOW_COND_JUMP );
}

void dasm_post_insn( void )
{
    UBYTE op0;

    if ( !read_opcode_byte( g_insn_addr, &op0 ) )
        return;

    switch ( op0 & 0x0F )
    {
    case 0x0A:
        dasm_cfg_set_flow( CFG_FLOW_COND_JUMP );
        break;

    case 0x0B:
    case 0x0D:
        z8_cfg_set_cond_jump_flow( (op0 >> 4) & 0x0F );
        break;

    case 0x0F:
        if ( op0 == 0x0F )
            dasm_cfg_set_flow( CFG_FLOW_STOP );
        else if ( op0 == 0x1F )
            dasm_cfg_set_flow( CFG_FLOW_HALT );
        break;
    }
}

static void reg4( unsigned int r )
{
    operand( "r%X", r & 0x0F );
}

static void reg8( UBYTE r )
{
    if ( r < 0x10 )
        operand( "r%X", r );
    else
        operand( FORMAT_NUM_8BIT, r );
}

static void ireg4( unsigned int r )
{
    operand( "@r%X", r & 0x0F );
}

static void ireg8( UBYTE r )
{
    operand( "@" );
    reg8( r );
}

static UWORD next_addr16( FILE *f, ADDR *addr )
{
    UBYTE msb = next( f, addr );
    UBYTE lsb = next( f, addr );

    return MK_WORD( lsb, msb );
}

/*****************************************************************************
 *        Private Functions
 *****************************************************************************/

OPERAND_FUNC(none)
{
    /* empty */
}

OPERAND_FUNC(reg_n)
{
    reg4( opc >> 4 );
}

OPERAND_FUNC(ireg_n)
{
    ireg4( opc >> 4 );
}

OPERAND_FUNC(reg_n_imm8)
{
    UBYTE imm = next( f, addr );

    reg4( opc >> 4 );
    operand( ",#" FORMAT_NUM_8BIT, imm );
}

OPERAND_FUNC(regpair_n)
{
    unsigned int r = opc & 0xF0;

    operand( "rr%X", r >> 4 );
}

OPERAND_FUNC(regpair_n_imm16)
{
    UWORD imm16 = next_addr16( f, addr );

    operand_regpair_n( f, addr, opc, xtype );
    operand( ",#" FORMAT_NUM_16BIT, imm16 );
}

OPERAND_FUNC(regpair_ind_n)
{
    unsigned int r = opc & 0xF0;

    operand( "@rr%X", r >> 4 );
}

OPERAND_FUNC(regpair_ind_n_imm16)
{
    UWORD imm16 = next_addr16( f, addr );

    operand_regpair_ind_n( f, addr, opc, xtype );
    operand( ",#" FORMAT_NUM_16BIT, imm16 );
}

OPERAND_FUNC(rr)
{
    UBYTE rb = next( f, addr );

    reg8( rb >> 4 );
    operand( "," );
    reg8( rb & 0x0F );
}

OPERAND_FUNC(r_ir)
{
    UBYTE rb = next( f, addr );

    reg8( rb >> 4 );
    operand( "," );
    ireg8( rb & 0x0F );
}

OPERAND_FUNC(r_R)
{
    UBYTE rb = next( f, addr );

    reg8( rb & 0x0F );
    operand( "," );
    reg4( rb >> 4 );
}

OPERAND_FUNC(ir_R)
{
    UBYTE rb = next( f, addr );

    ireg8( rb & 0x0F );
    operand( "," );
    reg4( rb >> 4 );
}

OPERAND_FUNC(r_imm8)
{
    UBYTE r = next( f, addr );
    UBYTE imm = next( f, addr );

    reg8( r );
    operand( ",#" FORMAT_NUM_8BIT, imm );
}

OPERAND_FUNC(ir_imm8)
{
    UBYTE r = next( f, addr );
    UBYTE imm = next( f, addr );

    ireg8( r );
    operand( ",#" FORMAT_NUM_8BIT, imm );
}

OPERAND_FUNC(djnz)
{
    BYTE disp;
    ADDR dest;

    reg4( opc >> 4 );
    operand( "," );
    disp = (BYTE)next( f, addr );
    dest = *addr + disp;
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( X_JMP, g_insn_addr, dest );
}

OPERAND_FUNC(jr)
{
    BYTE disp;
    ADDR dest;

    operand( "%s,", cc_names[(opc >> 4) & 0x0F] );
    disp = (BYTE)next( f, addr );
    dest = *addr + disp;
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( X_JMP, g_insn_addr, dest );
}

OPERAND_FUNC(jp)
{
    UWORD dest;

    operand( "%s,", cc_names[(opc >> 4) & 0x0F] );
    dest = next_addr16( f, addr );
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( X_JMP, g_insn_addr, dest );
}

OPERAND_FUNC(ext)
{
    UBYTE subop = ( opc >> 4 ) & 0x0F;

    if ( subop != 0 && subop != 1 )
    {
        UBYTE r = next( f, addr );

        if ( subop == 8 || subop == 10 )
            ireg8( r );
        else
            reg8( r );
    }
}

static const char *opcode_alu( OPC opc )
{
    return alu_ops[(opc >> 4) & 0x0F];
}

static const char *opcode_ext( OPC opc )
{
    return ext_ops[(opc >> 4) & 0x0F];
}

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
 * Instruction Decoding Tables
 *****************************************************************************/

optab_t base_optab[] = {
    MASK ( "dec",  reg_n,      0x0F, 0x00, X_REG )
    MASK ( "dec",  ireg_n,     0x0F, 0x01, X_PTR )

    MASK_DYN ( alu, rr,        0x0F, 0x02, X_REG )
    MASK_DYN ( alu, r_ir,      0x0F, 0x03, X_PTR )
    MASK_DYN ( alu, r_R,       0x0F, 0x04, X_REG )
    MASK_DYN ( alu, ir_R,      0x0F, 0x05, X_PTR )
    MASK_DYN ( alu, r_imm8,    0x0F, 0x06, X_IMM )
    MASK_DYN ( alu, ir_imm8,   0x0F, 0x07, X_IMM )

    MASK ( "ld",   rr,         0x0F, 0x08, X_REG )
    MASK ( "ld",   r_R,        0x0F, 0x09, X_REG )
    MASK ( "djnz", djnz,       0x0F, 0x0A, X_JMP )
    MASK ( "jr",   jr,         0x0F, 0x0B, X_JMP )
    MASK ( "ld",   reg_n_imm8, 0x0F, 0x0C, X_IMM )
    MASK ( "jp",   jp,         0x0F, 0x0D, X_JMP )
    MASK ( "inc",  reg_n,      0x0F, 0x0E, X_REG )

    INSN_DYN ( ext, ext, 0x0F, X_NONE )
    INSN_DYN ( ext, ext, 0x1F, X_NONE )
    INSN_DYN ( ext, ext, 0x2F, X_REG )
    INSN_DYN ( ext, ext, 0x4F, X_REG )
    INSN_DYN ( ext, ext, 0x5F, X_REG )
    INSN_DYN ( ext, ext, 0x6F, X_REG )
    INSN_DYN ( ext, ext, 0x7F, X_REG )
    INSN_DYN ( ext, ext, 0x8F, X_PTR )
    INSN_DYN ( ext, ext, 0x9F, X_REG )
    INSN_DYN ( ext, ext, 0xAF, X_PTR )
    INSN_DYN ( ext, ext, 0xBF, X_REG )
    INSN_DYN ( ext, ext, 0xCF, X_REG )
    INSN_DYN ( ext, ext, 0xDF, X_REG )
    INSN_DYN ( ext, ext, 0xEF, X_REG )
    INSN_DYN ( ext, ext, 0xFF, X_REG )

    END
};
