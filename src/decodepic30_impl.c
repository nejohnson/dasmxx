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

/*****************************************************************************
 * PIC24/dsPIC33 instructions are 24-bit words.  XC16 Intel HEX stores each
 * instruction in a 4-byte program-memory slot; the top byte is ignored here.
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "dasmxx.h"
#include "optab.h"

#ifndef PIC30_DASM_NAME
#error PIC30_DASM_NAME must be defined by the including wrapper
#endif

DASM_PROFILE( PIC30_DASM_NAME, PIC30_DASM_DESC, 4, 9, 0, 4, 2 )

#define FORMAT_NUM_16BIT        "$%04X"
#define FORMAT_NUM_24BIT        "$%06X"

static OPC next_slot( FILE *f, ADDR *addr )
{
    UBYTE lo = next( f, addr );
    UBYTE mid = next( f, addr );
    UBYTE hi = next( f, addr );

    (void)next( f, addr );
    return ((OPC)hi << 16) | ((OPC)mid << 8) | lo;
}

OPERAND_FUNC(none)
{
    /* empty */
}

static WORD sign_extend_16( UWORD v )
{
    return (WORD)v;
}

static void operand_wreg( unsigned int reg )
{
    operand( "W%u", reg & 0x0F );
}

static void operand_wind( unsigned int reg )
{
    operand( "[W%u]", reg & 0x0F );
}

OPERAND_FUNC(lit16_wd)
{
    UWORD lit = ( opc >> 4 ) & 0xFFFF;
    unsigned int wd = opc & 0x0F;

    operand( "#" FORMAT_NUM_16BIT, lit );
    COMMA;
    operand_wreg( wd );
}

OPERAND_FUNC(wb_ws_wd)
{
    unsigned int wb = opc & 0x0F;
    unsigned int wd = ( opc >> 7 ) & 0x0F;
    unsigned int ws = ( opc >> 15 ) & 0x0F;

    operand_wreg( ws );
    COMMA;
    operand_wreg( wb );
    COMMA;
    operand_wreg( wd );
}

OPERAND_FUNC(wsrc_wdst)
{
    unsigned int src = opc & 0x0F;
    unsigned int dst = ( opc >> 7 ) & 0x0F;

    if ( opc & BIT(4) )
        operand_wind( src );
    else
        operand_wreg( src );

    COMMA;

    if ( opc & BIT(11) )
        operand_wind( dst );
    else
        operand_wreg( dst );
}

OPERAND_FUNC(wb)
{
    operand_wreg( opc & 0x0F );
}

OPERAND_FUNC(wd)
{
    operand_wreg( ( opc >> 7 ) & 0x0F );
}

OPERAND_FUNC(wb_wd)
{
    unsigned int wb = opc & 0x0F;
    unsigned int wd = ( opc >> 7 ) & 0x0F;

    operand_wreg( wb );
    COMMA;
    operand_wreg( wd );
}

OPERAND_FUNC(cp_wb_ws)
{
    unsigned int wb = opc & 0x0F;
    unsigned int ws = ( opc >> 11 ) & 0x0F;

    operand_wreg( ws );
    COMMA;
    operand_wreg( wb );
}

OPERAND_FUNC(repeat_lit14)
{
    operand( "#$%X", opc & 0x3FFF );
}

OPERAND_FUNC(repeat_wb)
{
    operand_wreg( opc & 0x0F );
}

OPERAND_FUNC(rel16)
{
    WORD disp = sign_extend_16( opc & 0xFFFF );
    ADDR pc = g_insn_addr / dasm_word_width_bytes;
    ADDR dest = pc + 2 + ( disp * 2 );

    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

OPERAND_FUNC(addr23)
{
    OPC ext = next_slot( f, addr );
    ADDR dest = ( opc & 0xFFFF ) | ( ( ext & 0x007F ) << 16 );

    operand( xref_genwordaddr( NULL, FORMAT_NUM_24BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

static void operand_cond_rel( OPC opc, const char *cond, XREF_TYPE xtype )
{
    WORD disp = sign_extend_16( opc & 0xFFFF );
    ADDR pc = g_insn_addr / dasm_word_width_bytes;
    ADDR dest = pc + 2 + ( disp * 2 );

    operand( "%s", cond );
    COMMA;
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

OPERAND_FUNC(z_rel16)
{
    operand_cond_rel( opc, "Z", xtype );
}

OPERAND_FUNC(nz_rel16)
{
    operand_cond_rel( opc, "NZ", xtype );
}

OPERAND_FUNC(c_rel16)
{
    operand_cond_rel( opc, "C", xtype );
}

OPERAND_FUNC(nc_rel16)
{
    operand_cond_rel( opc, "NC", xtype );
}

OPERAND_FUNC(bit_wb)
{
    unsigned int bit = ( opc >> 12 ) & 0x0F;
    unsigned int wb = opc & 0x0F;

    operand_wreg( wb );
    COMMA;
    operand( "#$%X", bit );
}

OPERAND_FUNC(mac45_a)
{
    operand( "W4 * W5, A" );
}

OPERAND_FUNC(mac67_b)
{
    operand( "W6 * W7, B" );
}

optab_t base_optab[] = {
    INSN ( "NOP",    none,          0x000000,       X_NONE )
    INSN ( "RETURN", none,          0x060000,       X_NONE )
    INSN ( "RESET",  none,          0xFE0000,       X_NONE )
    INSN ( "BREAK",  none,          0xDA4000,       X_NONE )

    MASK ( "CLR.W",  wd,            0xFFF87F, 0xEB0000, X_NONE )
    MASK ( "CLR.B",  wd,            0xFFF87F, 0xEB4000, X_NONE )
    MASK ( "COM.W",  wb_wd,         0xFFF070, 0xEA8000, X_NONE )
    MASK ( "NEG.W",  wb_wd,         0xFFF070, 0xEA0000, X_NONE )
    MASK ( "INC.W",  wb_wd,         0xFF7070, 0xE80000, X_NONE )
    MASK ( "DEC.W",  wb_wd,         0xFF7070, 0xE90000, X_NONE )
    MASK ( "SL.W",   wb_wd,         0xFF7070, 0xD00000, X_NONE )
    MASK ( "LSR.W",  wb_wd,         0xFFF070, 0xD10000, X_NONE )
    MASK ( "ASR.W",  wb_wd,         0xFFF070, 0xD18000, X_NONE )

    MASK ( "MOV.W",  lit16_wd,      0xF00000, 0x200000, X_NONE )
    MASK ( "MOV.W",  wsrc_wdst,     0xFFF060, 0x780000, X_NONE )

    MASK ( "ADD.W",  wb_ws_wd,      0xF80000, 0x400000, X_NONE )
    MASK ( "SUB.W",  wb_ws_wd,      0xF80000, 0x500000, X_NONE )
    MASK ( "AND.W",  wb_ws_wd,      0xF80000, 0x600000, X_NONE )
    MASK ( "XOR.W",  wb_ws_wd,      0xF80000, 0x680000, X_NONE )
    MASK ( "IOR.W",  wb_ws_wd,      0xF80000, 0x700000, X_NONE )
    MASK ( "CP.W",   cp_wb_ws,      0xFF77F0, 0xE10000, X_NONE )

    MASK ( "BSET.W", bit_wb,        0xFF0000, 0xA00000, X_NONE )
    MASK ( "BCLR.W", bit_wb,        0xFF0000, 0xA10000, X_NONE )
    MASK ( "BTG.W",  bit_wb,        0xFF0000, 0xA20000, X_NONE )
    MASK ( "BTST.Z", bit_wb,        0xFF0000, 0xA30000, X_NONE )

    MASK ( "BRA",    rel16,         0xFF0000, 0x370000, X_JMP )
    MASK ( "RCALL",  rel16,         0xFF0000, 0x070000, X_CALL )
    MASK ( "GOTO",   addr23,        0xFF0000, 0x040000, X_JMP )
    MASK ( "CALL",   addr23,        0xFF0000, 0x020000, X_CALL )
    MASK ( "BRA",    z_rel16,       0xFF0000, 0x320000, X_JMP )
    MASK ( "BRA",    nz_rel16,      0xFF0000, 0x3A0000, X_JMP )
    MASK ( "BRA",    c_rel16,       0xFF0000, 0x310000, X_JMP )
    MASK ( "BRA",    nc_rel16,      0xFF0000, 0x390000, X_JMP )
    MASK ( "REPEAT", repeat_wb,     0xFFFFF0, 0x098000, X_NONE )
    MASK ( "REPEAT", repeat_lit14,  0xFF8000, 0x090000, X_NONE )

#ifdef PIC30_HAS_DSP
    INSN ( "MAC",    mac45_a,       0xC00112,       X_NONE )
    INSN ( "MAC",    mac67_b,       0xC68112,       X_NONE )
#endif

    END
};
