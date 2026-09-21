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

DASM_PROFILE( "dasm430", "TI MSP430", 8, 8, 0, 2, 1 )

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/

#define FORMAT_NUM_16BIT        "0x%04X"
#define FORMAT_NUM_20BIT        "0x%05X"

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

static int ext_active = 0;
static unsigned int ext_src = 0;
static unsigned int ext_dst = 0;
static unsigned int ext_al = 0;
static unsigned int ext_zc = 0;

static void addr16( UWORD a, XREF_TYPE xtype )
{
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, a ) );
    xref_addxref( xtype, g_insn_addr, a );
}

static void addr20( ADDR a, XREF_TYPE xtype )
{
    a &= 0xFFFFF;
    operand( xref_genwordaddr( NULL, FORMAT_NUM_20BIT, a ) );
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

static WORD next_signed_word( FILE *f, ADDR *addr )
{
    return (WORD)next_word( f, addr );
}

static void src_operand( FILE *f, ADDR *addr, unsigned int r, unsigned int mode,
                         XREF_TYPE xtype );
static void dst_operand( FILE *f, ADDR *addr, unsigned int r, unsigned int mode,
                         XREF_TYPE xtype );

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

static void indexed_addr20( FILE *f, ADDR *addr, unsigned int r, unsigned int ext,
                            XREF_TYPE xtype )
{
    WORD disp = next_signed_word( f, addr );
    ADDR a = ( ( ext & 0x0F ) << 16 ) | ( (UWORD)disp );

    if ( r == 0 )
    {
        ADDR target = *addr + disp + ( ( ext & 0x0F ) << 16 );
        addr20( target, xtype );
    }
    else if ( r == 2 )
    {
        operand( "&" );
        addr20( a, xtype );
    }
    else
    {
        operand( FORMAT_NUM_16BIT "(%s)", (UWORD)disp, regs[r & 0x0F] );
    }
}

static void emit_indexed_disp20( FILE *f, ADDR *addr, unsigned int r,
                                 unsigned int ext, XREF_TYPE xtype )
{
    WORD disp = next_signed_word( f, addr );
    ADDR value = ( ( ext & 0x0F ) << 16 ) | ( (UWORD)disp );

    if ( r == 0 )
    {
        ADDR target = *addr + ( ext_active ? -2 : 0 ) + (LWORD)value;
        addr20( target, xtype );
    }
    else if ( r == 2 )
    {
        operand( "&" );
        addr20( value, xtype );
    }
    else
    {
        if ( ext && ( value & 0x80000 ) )
            value |= ~0xFFFFF;
        operand( "0x%05X(%s)", value & 0xFFFFF, regs[r & 0x0F] );
    }
}

static void src_operand_ext_hi( FILE *f, ADDR *addr, unsigned int r,
                                unsigned int mode, unsigned int ext_hi,
                                XREF_TYPE xtype )
{
    if ( !ext_active || r == 3 || ( r == 2 && mode >= 2 ) )
    {
        src_operand( f, addr, r, mode, xtype );
        return;
    }

    if ( r == 2 && mode == 1 )
    {
        emit_indexed_disp20( f, addr, r, ext_hi, xtype );
        return;
    }

    switch ( mode & 0x03 )
    {
    case 0:
        reg( r );
        break;
    case 1:
        emit_indexed_disp20( f, addr, r, ext_hi, xtype );
        break;
    case 2:
        operand( "@%s", regs[r & 0x0F] );
        break;
    case 3:
        if ( r == 0 )
        {
            ADDR imm = ( ext_hi << 16 ) | next_word( f, addr );
            if ( xtype == X_CALL || xtype == X_JMP )
            {
                operand( "#" );
                addr20( imm, xtype );
            }
            else
            {
                operand( "#" FORMAT_NUM_20BIT, imm & 0xFFFFF );
            }
        }
        else
        {
            operand( "@%s+", regs[r & 0x0F] );
        }
        break;
    }
}

static void src_operand_ext( FILE *f, ADDR *addr, unsigned int r,
                             unsigned int mode, XREF_TYPE xtype )
{
    src_operand_ext_hi( f, addr, r, mode, ext_src, xtype );
}

static void dst_operand_ext( FILE *f, ADDR *addr, unsigned int r,
                             unsigned int mode, XREF_TYPE xtype )
{
    if ( !ext_active )
    {
        dst_operand( f, addr, r, mode, xtype );
        return;
    }

    if ( mode == 0 )
        reg( r );
    else
        emit_indexed_disp20( f, addr, r, ext_dst, xtype );
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

static const char *ext_suffix( OPC opc )
{
    if ( !ext_active )
        return ( opc & 0x0040 ) ? ".B" : "";

    if ( !ext_al )
        return ".A";

    return ( opc & 0x0040 ) ? ".B" : ".W";
}

static const char *opcode_with_ext_size( const char *base, OPC opc )
{
    static char text[16];

    if ( ext_active )
        sprintf( text, "%sX%s", base, ext_suffix( opc ) );
    else
        sprintf( text, "%s%s", base, ( opc & 0x0040 ) ? ".B" : "" );
    return text;
}

static const char *opcode_dual( OPC opc )
{
    return opcode_with_ext_size( dual_ops[( opc >> 12 ) & 0x0F], opc );
}

static const char *opcode_rrc( OPC opc )
{
    if ( ext_active && ext_zc )
        return opcode_with_ext_size( "RRU", opc );
    return opcode_with_ext_size( "RRC",  opc );
}

static const char *opcode_rra( OPC opc )  { return opcode_with_ext_size( "RRA",  opc ); }
static const char *opcode_push( OPC opc ) { return opcode_with_ext_size( "PUSH", opc ); }
static const char *opcode_swpb( OPC opc ) { return opcode_with_ext_size( "SWPB", opc ); }
static const char *opcode_sxt( OPC opc )  { return opcode_with_ext_size( "SXT",  opc ); }
static const char *opcode_call( OPC opc ) { return ext_active ? "CALLX.A" : "CALL"; }
static const char *opcode_jump( OPC opc ) { return jump_ops[( opc >> 10 ) & 0x07]; }

static const char *opcode_rrxm( OPC opc )
{
    static const char *names[] = { "RRCM", "RRAM", "RLAM", "RRUM" };
    static char text[16];

    sprintf( text, "%s.%c", names[( opc >> 8 ) & 0x03], ( opc & 0x0010 ) ? 'W' : 'A' );
    return text;
}

static const char *opcode_pushm( OPC opc )
{
    return ( opc & 0x0100 ) ? "PUSHM.W" : "PUSHM.A";
}

static const char *opcode_popm( OPC opc )
{
    return ( opc & 0x0100 ) ? "POPM.W" : "POPM.A";
}

static const char *opcode_alu_a( OPC opc )
{
    switch ( opc & 0x00B0 )
    {
    case 0x0090: return "CMPA";
    case 0x00A0: return "ADDA";
    case 0x00B0: return "SUBA";
    default:     return "???";
    }
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

#define MASK_DYN_CPU(M_opcode_fn, M_ops, M_mask, M_val, M_xt, M_min_cpu) \
    { .type      = OPTAB_MASK,                                           \
      .min_cpu   = M_min_cpu,                                            \
      .opcode    = "DYNAMIC",                                            \
      .opcode_fn = opcode_ ## M_opcode_fn,                                \
      .operands  = operand_ ## M_ops,                                     \
      .xtype     = M_xt,                                                  \
      .u.mask.mask = M_mask,                                              \
      .u.mask.val  = M_val                                                \
    },

/*****************************************************************************
 *        Private Functions
 *****************************************************************************/

void dasm_pre_insn( void )
{
    ext_active = 0;
    ext_src = 0;
    ext_dst = 0;
    ext_al = 0;
    ext_zc = 0;
}

PREFIX_FUNC(ext)
{
    (void)f;
    (void)addr;
    (void)xtype;

    ext_active = 1;
    ext_src = ( opc >> 7 ) & 0x0F;
    ext_al = ( opc >> 6 ) & 0x01;
    ext_zc = ( opc >> 8 ) & 0x01;
    ext_dst = opc & 0x0F;
}

OPERAND_FUNC(none)
{
    /* empty */
}

OPERAND_FUNC(single)
{
    unsigned int r = opc & 0x0F;
    unsigned int as = ( opc >> 4 ) & 0x03;

    src_operand_ext_hi( f, addr, r, as, ext_dst, xtype );
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

    src_operand_ext( f, addr, src, as, xtype );
    operand( "," );
    dst_operand_ext( f, addr, dst, ad, xtype );
}

OPERAND_FUNC(rrxm)
{
    unsigned int count = ( ( opc >> 10 ) & 0x03 ) + 1;
    unsigned int dst = opc & 0x0F;

    operand( "#%u,", count );
    reg( dst );
}

OPERAND_FUNC(pushm)
{
    unsigned int count = ( ( opc >> 4 ) & 0x0F ) + 1;
    unsigned int src = opc & 0x0F;

    operand( "#%u,", count );
    reg( src );
}

OPERAND_FUNC(popm)
{
    unsigned int count = ( ( opc >> 4 ) & 0x0F ) + 1;
    unsigned int dst = ( opc & 0x0F ) + count - 1;

    operand( "#%u,", count );
    reg( dst );
}

OPERAND_FUNC(alu_a)
{
    unsigned int src = ( opc >> 8 ) & 0x0F;
    unsigned int dst = opc & 0x0F;

    if ( opc & 0x0040 )
        reg( src );
    else
    {
        ADDR imm = ( src << 16 ) | next_word( f, addr );
        operand( "#" FORMAT_NUM_20BIT, imm );
    }

    operand( "," );
    reg( dst );
}

OPERAND_FUNC(mova)
{
    unsigned int src = ( opc >> 8 ) & 0x0F;
    unsigned int mode = ( opc >> 4 ) & 0x0F;
    unsigned int dst = opc & 0x0F;

    switch ( mode )
    {
    case 0:
        operand( "@%s", regs[src] );
        operand( "," );
        reg( dst );
        break;
    case 1:
        operand( "@%s+", regs[src] );
        operand( "," );
        reg( dst );
        break;
    case 2:
        operand( "&" );
        addr20( ( src << 16 ) | next_word( f, addr ), xtype );
        operand( "," );
        reg( dst );
        break;
    case 3:
        indexed_addr20( f, addr, src, 0, xtype );
        operand( "," );
        reg( dst );
        break;
    case 6:
        reg( src );
        operand( ",&" );
        addr20( ( dst << 16 ) | next_word( f, addr ), xtype );
        break;
    case 7:
        reg( src );
        operand( "," );
        indexed_addr20( f, addr, dst, 0, xtype );
        break;
    case 8:
        operand( "#" FORMAT_NUM_20BIT, ( src << 16 ) | next_word( f, addr ) );
        operand( "," );
        reg( dst );
        break;
    case 12:
        reg( src );
        operand( "," );
        reg( dst );
        break;
    }
}

OPERAND_FUNC(calla)
{
    unsigned int mode = ( opc >> 4 ) & 0x0F;
    unsigned int regnum = opc & 0x0F;

    switch ( mode )
    {
    case 4:
        reg( regnum );
        break;
    case 5:
        indexed_addr20( f, addr, regnum, 0, X_CALL );
        break;
    case 6:
        operand( "@%s", regs[regnum] );
        break;
    case 7:
        operand( "@%s+", regs[regnum] );
        break;
    case 8:
        operand( "&" );
        addr20( ( regnum << 16 ) | next_word( f, addr ), X_CALL );
        break;
    case 9:
        {
            WORD disp = next_signed_word( f, addr );
            ADDR target = *addr + disp + ( regnum << 16 );
            addr20( target, X_CALL );
        }
        break;
    case 11:
        operand( "#" );
        addr20( ( regnum << 16 ) | next_word( f, addr ), X_CALL );
        break;
    default:
        operand( "?" );
        break;
    }
}

/*****************************************************************************
 * Instruction Decoding Tables
 *****************************************************************************/

optab_t base_optab[] = {
    MASK_PREFIX_CPU ( ext, 0xF800, 0x1800, CPU_MSP430X )

    /*
     * MSP430X address instructions that do not use the extension word.
     */
    MASK_CPU ( "RETA",  none,  0xFFFF, 0x0110, X_NONE, CPU_MSP430X )
    MASK_DYN_CPU ( rrxm,  rrxm,  0xF3E0, 0x0040, X_NONE, CPU_MSP430X )
    MASK_DYN_CPU ( rrxm,  rrxm,  0xF3E0, 0x0140, X_NONE, CPU_MSP430X )
    MASK_DYN_CPU ( rrxm,  rrxm,  0xF3E0, 0x0240, X_NONE, CPU_MSP430X )
    MASK_DYN_CPU ( rrxm,  rrxm,  0xF3E0, 0x0340, X_NONE, CPU_MSP430X )
    MASK_DYN_CPU ( alu_a, alu_a, 0xF0B0, 0x0090, X_IMM,  CPU_MSP430X )
    MASK_DYN_CPU ( alu_a, alu_a, 0xF0B0, 0x00A0, X_IMM,  CPU_MSP430X )
    MASK_DYN_CPU ( alu_a, alu_a, 0xF0B0, 0x00B0, X_IMM,  CPU_MSP430X )
    MASK_CPU     ( "MOVA", mova,  0xF0E0, 0x0000, X_PTR,  CPU_MSP430X )
    MASK_CPU     ( "MOVA", mova,  0xF0E0, 0x0020, X_PTR,  CPU_MSP430X )
    MASK_CPU     ( "MOVA", mova,  0xF0E0, 0x0060, X_PTR,  CPU_MSP430X )
    MASK_CPU     ( "MOVA", mova,  0xF0F0, 0x0080, X_IMM,  CPU_MSP430X )
    MASK_CPU     ( "MOVA", mova,  0xF0F0, 0x00C0, X_REG,  CPU_MSP430X )
    MASK_CPU     ( "CALLA", calla, 0xFFC0, 0x1340, X_CALL, CPU_MSP430X )
    MASK_CPU     ( "CALLA", calla, 0xFFF0, 0x1380, X_CALL, CPU_MSP430X )
    MASK_CPU     ( "CALLA", calla, 0xFFF0, 0x1390, X_CALL, CPU_MSP430X )
    MASK_CPU     ( "CALLA", calla, 0xFFF0, 0x13B0, X_CALL, CPU_MSP430X )
    MASK_DYN_CPU ( pushm, pushm, 0xFE00, 0x1400, X_NONE, CPU_MSP430X )
    MASK_DYN_CPU ( popm,  popm,  0xFE00, 0x1600, X_NONE, CPU_MSP430X )

    /*
     * Format II: single-operand instructions.
     */
    INSN ( "RETI", none,   0x1300, X_NONE )
    MASK_DYN ( rrc,  single, 0xFF80, 0x1000, X_NONE )
    MASK_DYN ( swpb, single, 0xFF80, 0x1080, X_NONE )
    MASK_DYN ( rra,  single, 0xFF80, 0x1100, X_NONE )
    MASK_DYN ( sxt,  single, 0xFF80, 0x1180, X_NONE )
    MASK_DYN ( push, single, 0xFF80, 0x1200, X_NONE )
    MASK_DYN ( call, single, 0xFF80, 0x1280, X_CALL )

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
