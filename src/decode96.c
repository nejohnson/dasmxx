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

#include "dasmxx.h"
#include "optab.h"

DASM_PROFILE( "dasm96", "Intel 8096", 8, 9, 0, 1, 1 )

#define FORMAT_NUM_16BIT        "%04X"

enum {
    ADDR_DIRECT = 0,
    ADDR_IMMED  = 1,
    ADDR_INDIR  = 2,
    ADDR_INDEX  = 3
};

static int signed_prefix = 0;

static UWORD make_word( UBYTE low, UBYTE high )
{
    return (UWORD)( low | ( high << 8 ) );
}

static UWORD next_word( FILE *f, ADDR *addr )
{
    UBYTE low  = next( f, addr );
    UBYTE high = next( f, addr );

    return make_word( low, high );
}

static WORD next_offset( FILE *f, ADDR *addr )
{
    return (WORD)next_word( f, addr );
}

static int middle_op( OPC opc )
{
    int op = 0;

    if ( opc & 0x80 )
        op |= 8;
    if ( opc & 0x20 )
        op |= 4;
    if ( opc & 0x08 )
        op |= 2;
    if ( opc & 0x04 )
        op |= 1;

    return op;
}

static int middle_is_three_operand( OPC opc )
{
    return ( opc & 0xE0 ) == 0x40;
}

static ADDR rel8_target( ADDR next_addr, UBYTE disp )
{
    return next_addr + (BYTE)disp;
}

static ADDR rel16_target( ADDR next_addr, WORD disp )
{
    return next_addr + disp;
}

int dasm_cfg_supported( void )
{
    return 1;
}

static int read_opcode_byte( ADDR addr, UBYTE *out )
{
    return dasm_input_read_byte_at( addr, out );
}

void dasm_post_insn( void )
{
    UBYTE op0;
    ADDR opaddr = g_insn_addr;

    if ( !read_opcode_byte( opaddr, &op0 ) )
        return;

    if ( op0 == 0xFE )
    {
        opaddr++;
        if ( !read_opcode_byte( opaddr, &op0 ) )
            return;
    }

    if ( op0 >= 0x20 && op0 <= 0x27 )
        dasm_cfg_set_flow( CFG_FLOW_JUMP );
    else if ( op0 >= 0x28 && op0 <= 0x2F )
        dasm_cfg_set_flow( CFG_FLOW_CALL );
    else if ( (op0 >= 0x30 && op0 <= 0x3F)
              || (op0 >= 0xD0 && op0 <= 0xDF)
              || op0 == 0xE0
              || op0 == 0xE1 )
        dasm_cfg_set_flow( CFG_FLOW_COND_JUMP );
    else if ( op0 == 0xE3 )
        dasm_cfg_set_flow( CFG_FLOW_INDIRECT_JUMP );
    else if ( op0 == 0xE7 )
        dasm_cfg_set_flow( CFG_FLOW_JUMP );
    else if ( op0 == 0xEF )
        dasm_cfg_set_flow( CFG_FLOW_CALL );
    else if ( op0 == 0xF0 )
        dasm_cfg_set_flow( CFG_FLOW_RETURN );
    else if ( op0 == 0xF6 )
        dasm_cfg_set_flow( CFG_FLOW_HALT );
    else if ( op0 == 0xFF )
        dasm_cfg_set_flow( CFG_FLOW_STOP );
}

void dasm_pre_insn( void )
{
    signed_prefix = 0;
}

PREFIX_FUNC(signed)
{
    (void)f;
    (void)addr;
    (void)opc;
    (void)xtype;

    signed_prefix = 1;
}

OPERAND_FUNC(none)
{
    (void)f;
    (void)addr;
    (void)opc;
    (void)xtype;
}

OPERAND_FUNC(ignore8)
{
    (void)opc;
    (void)xtype;

    (void)next( f, addr );
}

OPERAND_FUNC(reg)
{
    UBYTE reg = next( f, addr );

    operand( "R%02X", reg );
}

OPERAND_FUNC(reg_ind)
{
    UBYTE reg = next( f, addr );

    operand( "[R%02X]", reg );
}

OPERAND_FUNC(reg_reg_80196)
{
    UBYTE dst = next( f, addr );
    UBYTE src = next( f, addr );

    operand( "R%02X, R%02X", dst, src );
}

OPERAND_FUNC(sjmp)
{
    UBYTE disp = next( f, addr );
    WORD offset = (WORD)( ( ( opc & 0x03 ) << 8 ) | disp );
    ADDR target;

    if ( opc & 0x04 )
        offset |= (WORD)0xFC00;

    target = *addr + offset;
    operand( "%s", xref_genwordaddr( NULL, FORMAT_NUM_16BIT, target ) );
    xref_addxref( xtype, g_insn_addr, target );
}

OPERAND_FUNC(bit_rel)
{
    UBYTE src = next( f, addr );
    UBYTE disp = next( f, addr );
    ADDR target = rel8_target( *addr, disp );

    operand( "R%02X,%d, %s", src, opc & 0x07,
             xref_genwordaddr( NULL, FORMAT_NUM_16BIT, target ) );
    xref_addxref( xtype, g_insn_addr, target );
}

OPERAND_FUNC(rel8)
{
    UBYTE disp = next( f, addr );
    ADDR target = rel8_target( *addr, disp );

    operand( "%s", xref_genwordaddr( NULL, FORMAT_NUM_16BIT, target ) );
    xref_addxref( xtype, g_insn_addr, target );
}

OPERAND_FUNC(reg_rel8)
{
    UBYTE reg = next( f, addr );
    UBYTE disp = next( f, addr );
    ADDR target = rel8_target( *addr, disp );

    operand( "R%02X, %s", reg, xref_genwordaddr( NULL, FORMAT_NUM_16BIT, target ) );
    xref_addxref( xtype, g_insn_addr, target );
}

OPERAND_FUNC(rel16)
{
    WORD disp = next_offset( f, addr );
    ADDR target = rel16_target( *addr, disp );

    operand( "%s", xref_genwordaddr( NULL, FORMAT_NUM_16BIT, target ) );
    xref_addxref( xtype, g_insn_addr, target );
}

OPERAND_FUNC(op00)
{
    UBYTE arg1 = next( f, addr );

    if ( opc & 0x08 )
    {
        UBYTE dst = next( f, addr );

        operand( "R%02X, ", dst );
        if ( opc != 0x0F && arg1 < 0x10 )
            operand( "#%02X", arg1 );
        else
            operand( "R%02X", arg1 );
    }
    else
    {
        operand( "R%02X", arg1 );
    }
}

OPERAND_FUNC(middle)
{
    int op = middle_op( opc );
    int three_op = middle_is_three_operand( opc );

    switch ( opc & 0x03 )
    {
    case ADDR_DIRECT:
        {
            UBYTE src = next( f, addr );
            UBYTE dst_or_src2 = next( f, addr );

            if ( three_op )
            {
                UBYTE dst = next( f, addr );
                operand( "R%02X, R%02X, R%02X", dst, dst_or_src2, src );
            }
            else
            {
                operand( "R%02X, R%02X", dst_or_src2, src );
            }
        }
        break;

    case ADDR_IMMED:
        if ( ( opc & 0x10 ) || op == 0x0F )
        {
            UBYTE imm = next( f, addr );
            UBYTE dst_or_src = next( f, addr );

            if ( three_op )
            {
                UBYTE dst = next( f, addr );
                operand( "R%02X, ", dst );
            }

            operand( "R%02X, #%02X", dst_or_src, imm );
        }
        else
        {
            UWORD imm = next_word( f, addr );
            UBYTE dst_or_src = next( f, addr );

            if ( three_op )
            {
                UBYTE dst = next( f, addr );
                operand( "R%02X, ", dst );
            }

            operand( "R%02X, #%s", dst_or_src,
                     xref_genwordaddr( NULL, FORMAT_NUM_16BIT, imm ) );
            xref_addxref( X_DATA, g_insn_addr, imm );
        }
        break;

    case ADDR_INDIR:
        {
            UBYTE ptr = next( f, addr );
            UBYTE dst_or_src = next( f, addr );

            if ( three_op )
            {
                UBYTE dst = next( f, addr );
                operand( "R%02X, ", dst );
            }

            operand( "R%02X, [R%02X]", dst_or_src, ptr & 0xFE );
            if ( ptr & 0x01 )
                operand( "+" );
        }
        break;

    case ADDR_INDEX:
        {
            UBYTE ptr = next( f, addr );

            if ( ptr & 0x01 )
            {
                UWORD disp = next_word( f, addr );
                UBYTE dst_or_src = next( f, addr );

                if ( three_op )
                {
                    UBYTE dst = next( f, addr );
                    operand( "R%02X, R%02X, ", dst, dst_or_src );
                }
                else
                {
                    operand( "R%02X, ", dst_or_src );
                }

                operand( "%s[R%02X]", xref_genwordaddr( NULL, FORMAT_NUM_16BIT, disp ),
                         ptr & 0xFE );
                xref_addxref( X_PTR, g_insn_addr, disp );
            }
            else
            {
                UBYTE disp = next( f, addr );
                UBYTE dst_or_src = next( f, addr );

                if ( three_op )
                {
                    UBYTE dst = next( f, addr );
                    operand( "R%02X, R%02X, ", dst, dst_or_src );
                }
                else
                {
                    operand( "R%02X, ", dst_or_src );
                }

                operand( "%02X[R%02X]", disp, ptr & 0xFE );
            }
        }
        break;
    }
}

OPERAND_FUNC(store_direct)
{
    UBYTE src = next( f, addr );
    UBYTE dst = next( f, addr );

    operand( "R%02X, R%02X", dst, src );
}

OPERAND_FUNC(store_indir)
{
    UBYTE ptr = next( f, addr );

    operand( "[R%02X]", ptr & 0xFE );
    if ( ptr & 0x01 )
        operand( "+" );
}

OPERAND_FUNC(store_index)
{
    UBYTE ptr = next( f, addr );

    if ( ptr & 0x01 )
    {
        UWORD disp = next_word( f, addr );
        UBYTE src = next( f, addr );

        operand( "R%02X, %s[R%02X]", src,
                 xref_genwordaddr( NULL, FORMAT_NUM_16BIT, disp ), ptr & 0xFE );
        xref_addxref( X_PTR, g_insn_addr, disp );
    }
    else
    {
        UBYTE disp = next( f, addr );
        UBYTE src = next( f, addr );

        operand( "R%02X, %02X[R%02X]", src, disp, ptr & 0xFE );
    }
}

OPERAND_FUNC(push_imm16)
{
    UWORD imm = next_word( f, addr );

    operand( "#%s", xref_genwordaddr( NULL, FORMAT_NUM_16BIT, imm ) );
}

OPERAND_FUNC(pushpop_index)
{
    UBYTE ptr = next( f, addr );

    if ( ptr & 0x01 )
    {
        UWORD disp = next_word( f, addr );

        operand( "%s[R%02X]", xref_genwordaddr( NULL, FORMAT_NUM_16BIT, disp ), ptr & 0xFE );
        xref_addxref( X_PTR, g_insn_addr, disp );
    }
    else
    {
        UBYTE disp = next( f, addr );

        operand( "%02X[R%02X]", disp, ptr & 0xFE );
    }
}

static const char *opcode_00( OPC opc )
{
    static const char *opcodes[] = {
        "skip",     "clr",      "not",      "neg",
        "",         "dec",      "ext",      "inc",
        "shr",      "shl",      "shra",     "",
        "shrl",     "shll",     "shral",    "norml",

        "",         "clrb",     "notb",     "negb",
        "",         "decb",     "extb",     "incb",
        "shrb",     "shlb",     "shrab",    "",
        "",         "",         "",         ""
    };

    return opcodes[opc & 0x1F];
}

static const char *opcode_middle( OPC opc )
{
    static char buf[16];
    static const char *opcodes[] = {
        "and",      "add",      "sub",      "mul",
        "and",      "add",      "sub",      "mul",
        "or",       "xor",      "cmp",      "div",
        "ld",       "addc",     "subc",     "ldbse"
    };
    int op = middle_op( opc );

    if ( op == 0x0F )
        return ( opc & 0x10 ) ? "ldbse" : "ldbze";

    if ( op == 0x03 || op == 0x0B )
    {
        snprintf( buf, sizeof(buf), "%s%s%s",
                  opcodes[op],
                  signed_prefix ? "" : "u",
                  ( opc & 0x10 ) ? "b" : "" );
    }
    else
    {
        snprintf( buf, sizeof(buf), "%s%s", opcodes[op], ( opc & 0x10 ) ? "b" : "" );
    }

    return buf;
}

static const char *opcode_condjmp( OPC opc )
{
    static const char *opcodes[] = {
        "jnst",     "jnh",      "jgt",      "jnc",
        "jnvt",     "jnv",      "jge",      "jne",
        "jst",      "jh",       "jle",      "jc",
        "jvt",      "jv",       "jlt",      "je"
    };

    return opcodes[opc & 0x0F];
}

optab_t base_optab[] = {

    PREFIX( signed, 0xFE )

    UNDEF( 0x10 )
    UNDEF( 0x14 )
    UNDEF( 0x1B )
    UNDEF( 0x1C )
    UNDEF( 0x1D )
    UNDEF( 0x1E )
    UNDEF( 0x1F )
    RANGE_DYN( 00, op00, 0x00, 0x1F, X_NONE )

    RANGE( "sjmp",  sjmp,    0x20, 0x27, X_JMP )
    RANGE( "scall", sjmp,    0x28, 0x2F, X_CALL )
    RANGE( "jbc",   bit_rel, 0x30, 0x37, X_JMP )
    RANGE( "jbs",   bit_rel, 0x38, 0x3F, X_JMP )

    RANGE_DYN( middle, middle, 0x40, 0xBF, X_NONE )

    INSN( "st",   store_direct,  0xC0, X_NONE )
    INSN( "bmov", reg_reg_80196, 0xC1, X_NONE )
    INSN( "st",   store_indir,   0xC2, X_NONE )
    INSN( "st",   store_index,   0xC3, X_NONE )
    INSN( "stb",  store_direct,  0xC4, X_NONE )
    INSN( "cmpl", reg_reg_80196, 0xC5, X_NONE )
    INSN( "stb",  store_indir,   0xC6, X_NONE )
    INSN( "stb",  store_index,   0xC7, X_NONE )
    INSN( "push", reg,           0xC8, X_NONE )
    INSN( "push", push_imm16,    0xC9, X_NONE )
    INSN( "push", store_indir,   0xCA, X_NONE )
    INSN( "push", pushpop_index, 0xCB, X_NONE )
    INSN( "pop",  reg,           0xCC, X_NONE )
    INSN( "pop",  store_indir,   0xCE, X_NONE )
    INSN( "pop",  pushpop_index, 0xCF, X_NONE )

    RANGE_DYN( condjmp, rel8, 0xD0, 0xDF, X_JMP )

    INSN( "djnz",  reg_rel8, 0xE0, X_JMP )
    INSN( "djnzw", reg_rel8, 0xE1, X_JMP )
    INSN( "br",    reg_ind,  0xE3, X_NONE )
    INSN( "ljmp",  rel16,    0xE7, X_JMP )
    INSN( "lcall", rel16,    0xEF, X_CALL )

    INSN( "ret",   none,    0xF0, X_NONE )
    INSN( "pushf", none,    0xF2, X_NONE )
    INSN( "popf",  none,    0xF3, X_NONE )
    INSN( "pusha", none,    0xF4, X_NONE )
    INSN( "popa",  none,    0xF5, X_NONE )
    INSN( "idlpd", ignore8, 0xF6, X_NONE )
    INSN( "clrc",  none,    0xF8, X_NONE )
    INSN( "setc",  none,    0xF9, X_NONE )
    INSN( "di",    none,    0xFA, X_NONE )
    INSN( "ei",    none,    0xFB, X_NONE )
    INSN( "clrvt", none,    0xFC, X_NONE )
    INSN( "nop",   none,    0xFD, X_NONE )
    INSN( "rst",   none,    0xFF, X_NONE )

    END
};
