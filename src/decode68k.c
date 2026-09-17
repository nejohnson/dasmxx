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
 
 /*****************************************************************************
 *   68000 INSTRUCTION SET (upto and including the 68030)
 * 
 *   As documented in Motorola document M68000PM/AD rev.1
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

DASM_PROFILE( "dasm68k", "Motorola 68000", 22, 10, 1, 2, 1 )

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/

/* Common output formats */
#define FORMAT_IMM8             "$%02X"
#define FORMAT_IMM16            "$%04X"
#define FORMAT_IMM32            "$%08X"

#define FORMAT_AREG             "A%d"
#define FORMAT_DREG             "D%d"
#define FORMAT_VECTOR           "#%d"

/* Construct a 32-bit long word out of two 1-bit words */
#define MK_LONG(h,l)            ( ( (l) & 0xFFFF)        \
                                | (((h) & 0xFFFF) << 16) )

enum {
    OPSIZE_BYTE = 1,
    OPSIZE_WORD = 2,
    OPSIZE_LONG = 4,
    OPSIZE_DOUBLE = 8,
    OPSIZE_EXTENDED = 12
};

static LWORD abs_lword( LWORD value )
{
    return value < 0 ? -value : value;
}
                                
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
 * Process address register operands.
 *    register number comes from bits 2:0 in opc
 ************************************************************/
OPERAND_FUNC(areg0)
{
    int reg = opc & 0x07;

    operand( FORMAT_AREG, reg );
}

/***********************************************************
 * Process address register operands.
 *    register number comes from bits 11:9 in opc
 ************************************************************/
OPERAND_FUNC(areg9)
{
    int reg = ( opc >> 9 ) & 0x07;

    operand( FORMAT_AREG, reg );
}

/***********************************************************
 * Process data register operands.
 *    register number comes from bits 2:0 in opc
 ************************************************************/
OPERAND_FUNC(dreg0)
{
    int reg = opc & 0x07;

    operand( FORMAT_DREG, reg );
}

/***********************************************************
 * Process data register operands.
 *    register number comes from bits 11:9 in opc
 ************************************************************/
OPERAND_FUNC(dreg9)
{
    int reg = ( opc >> 9 ) & 0x07;
    
    operand( FORMAT_DREG, reg );
}

OPERAND_FUNC(dreg0_dreg9)
{
    operand( FORMAT_DREG ", " FORMAT_DREG, opc & 0x07, (opc >> 9) & 0x07 );
}

OPERAND_FUNC(predec0_predec9)
{
    operand( "-(" FORMAT_AREG "), -(" FORMAT_AREG ")",
             opc & 0x07, (opc >> 9) & 0x07 );
}

OPERAND_FUNC(postinc0_postinc9)
{
    operand( "(" FORMAT_AREG ")+, (" FORMAT_AREG ")+",
             opc & 0x07, (opc >> 9) & 0x07 );
}

/***********************************************************
 * Process 3-bit vector operands.
 *    vector number comes from bits 2:0 in opc
 ************************************************************/
OPERAND_FUNC(vector3)
{
    int vector = opc & 0x07;
    
    operand( FORMAT_VECTOR, vector );
}

/***********************************************************
 * Process 4-bit vector operands.
 *    vector number comes from bits 3:0 in opc
 ************************************************************/
OPERAND_FUNC(vector4)
{
    int vector = opc & 0x0F;
    
    operand( FORMAT_VECTOR, vector );
}

/***********************************************************
 * Process "imm16" operand.
 ************************************************************/
OPERAND_FUNC(imm16)
{
    UWORD imm16 = (UWORD)nextw( f, addr );

    operand( "#" FORMAT_IMM16, imm16 );
}

/***********************************************************
 * Process 8-bit signed immediate stored in opc
 ************************************************************/
OPERAND_FUNC(simm8)
{
    WORD imm = (WORD)(opc & 0xFF);
    
    if ( imm > 0x7F )
        imm -= 0x100;
    
    operand( "%s#" FORMAT_IMM8, imm < 0 ? "-" : "", abs(imm) );
}

/***********************************************************
 * Process 16-bit signed immediate
 ************************************************************/
OPERAND_FUNC(simm16)
{
    WORD imm = (WORD)nextw( f, addr );
    
    operand( "%s#" FORMAT_IMM16, imm < 0 ? "-" : "", abs(imm) );
}

/***********************************************************
 * Process 32-bit signed immediate
 ************************************************************/
OPERAND_FUNC(simm32)
{
    LWORD imm = (LWORD)nextw( f, addr );
    imm = ( imm << 16 ) | (UWORD)nextw( f, addr );
    
    operand( "%s#" FORMAT_IMM32, imm < 0 ? "-" : "", abs_lword( imm ) );
}

/************************************************************
 * Process variable-length relative address
 ************************************************************/
OPERAND_FUNC(relX)
{
    BYTE disp8 = (BYTE)(opc & 0xFF);
    WORD dest = (ADDR)disp8;

    if ( disp8 == 0 || ( disp8 == -1 && ( dasm_cpu_level == 0 || dasm_cpu_level >= 68020 ) ) )
    {
        dest = (WORD)nextw( f, addr );
        if ( disp8 == -1 )
        {
            UWORD lo = (UWORD)nextw( f, addr );
            dest = MK_LONG(dest, lo);
        }
    }

    dest += *addr;
    
    operand( xref_genwordaddr( NULL, FORMAT_IMM32, dest ) );
    xref_addxref( xtype, g_insn_addr, dest ); 
}

/************************************************************
 * Process 6-bit effective address:
 * 5  4  3  2  1  0
 * [ reg  ][ mode ]
 * 
 * Reg:  address or data register 0-7, or submode if mode = 111
 * Mode: specifies effective address mode, which may also
 *        require additional words for constant fields
 ************************************************************/
enum {
    EAMODE_DATA_DIRECT   = 0x00,
    EAMODE_ADDR_DIRECT   = 0x01,
    EAMODE_ADDR_INDIR    = 0x02,
    EAMODE_ADDR_POST_INC = 0x03,
    EAMODE_ADDR_PRE_DEC  = 0x04,
    EAMODE_ADDR_IND_DISP = 0x05,
    EAMODE_ADDR_IND_IDX  = 0x06,
    EAMODE_SUB_MODE      = 0x07
};

static LWORD read_s16( FILE *f, ADDR *addr )
{
    return (WORD)nextw( f, addr );
}

static LWORD read_s32( FILE *f, ADDR *addr )
{
    UWORD hi = nextw( f, addr );
    UWORD lo = nextw( f, addr );

    return (LWORD)MK_LONG( hi, lo );
}

static void emit_displacement( LWORD disp, const char *fmt )
{
    operand( "%s", disp < 0 ? "-" : "" );
    operand( fmt, disp < 0 ? -disp : disp );
}

static void emit_abs_addr( ADDR dest, const char *fmt, XREF_TYPE xtype )
{
    operand( xref_genwordaddr( NULL, fmt, dest ) );
    if ( xtype != X_NONE )
        xref_addxref( xtype, g_insn_addr, dest );
}

static void emit_indexed_ea( FILE *f, ADDR *addr, int base_reg, int pc_relative )
{
    UWORD extn = nextw( f, addr );
    int da     = extn & (1 << 15);
    int ireg   = (extn >> 12) & 0x07;
    int wl     = extn & (1 << 11);
    int scale  = (extn >> 9) & 0x03;
    char base[16];

    if ( pc_relative )
        sprintf( base, "PC" );
    else
        sprintf( base, FORMAT_AREG, base_reg );

    if ( extn & 0x0100 )
    {
        int bs      = extn & (1 << 7);
        int is      = extn & (1 << 6);
        int bd_size = (extn >> 4) & 0x03;
        int iis     = extn & 0x07;
        int od_size = iis & 0x03;
        LWORD bd    = 0;
        LWORD od    = 0;

        if ( bd_size == 0x02 )
            bd = read_s16( f, addr );
        else if ( bd_size == 0x03 )
            bd = read_s32( f, addr );

        if ( od_size == 0x02 )
            od = read_s16( f, addr );
        else if ( od_size == 0x03 )
            od = read_s32( f, addr );

        operand( "(" );
        emit_displacement( bd, FORMAT_IMM32 );
        if ( !bs )
            operand( ",%s", base );
        if ( !is )
            operand( ",%c%d.%c*%d", da ? 'A' : 'D', ireg, wl ? 'L' : 'W', (1 << scale) );
        if ( od_size != 0 )
        {
            operand( "," );
            emit_displacement( od, FORMAT_IMM32 );
        }
        operand( ")" );
    }
    else
    {
        BYTE disp = (BYTE)(extn & 0xFF);

        operand( "(" );
        emit_displacement( disp, FORMAT_IMM8 );
        operand( ",%s,%c%d.%c*%d)", base, da ? 'A' : 'D', ireg, wl ? 'L' : 'W', (1 << scale) );
    }
}

static void emit_imm_ea( FILE *f, ADDR *addr, int size )
{
    if ( size == OPSIZE_BYTE )
        operand( "#" FORMAT_IMM8, (UWORD)nextw( f, addr ) & 0xFF );
    else if ( size == OPSIZE_LONG )
        operand( "#" FORMAT_IMM32, (ULWORD)read_s32( f, addr ) );
    else if ( size == OPSIZE_DOUBLE || size == OPSIZE_EXTENDED )
    {
        int words = size / 2;
        int i;

        operand( "#$" );
        for ( i = 0; i < words; i++ )
            operand( "%04X", (UWORD)nextw( f, addr ) );
    }
    else
        operand( "#" FORMAT_IMM16, (UWORD)nextw( f, addr ) );
}

static void emit_ea_field( FILE *f, ADDR *addr, int ea, int size, XREF_TYPE xtype )
{
    int mode = (ea >> 3) & 0x7;
    int reg  = ea & 0x7;
    
    switch( mode )
    {
    case EAMODE_DATA_DIRECT:                        /* 2.2.1 */
        operand( FORMAT_DREG, reg );
        break;
        
    case EAMODE_ADDR_DIRECT:                        /* 2.2.2 */
        operand( FORMAT_AREG, reg );
        break;
        
    case EAMODE_ADDR_INDIR:                         /* 2.2.3 */
        operand( "(" FORMAT_AREG ")", reg );
        break;
        
    case EAMODE_ADDR_POST_INC:                      /* 2.2.4 */
        operand( "(" FORMAT_AREG ")+", reg );
        break;
        
    case EAMODE_ADDR_PRE_DEC:                       /* 2.2.5 */
        operand( "-(" FORMAT_AREG ")", reg );
        break;
        
    case EAMODE_ADDR_IND_DISP:                      /* 2.2.6 */
    {
        LWORD disp = read_s16( f, addr );
        operand( "(" );
        emit_displacement( disp, FORMAT_IMM16 );
        operand( "," FORMAT_AREG ")", reg );
        break;
    }
    
    case EAMODE_ADDR_IND_IDX:                       /* 2.2.7 - 2.2.10 */
        emit_indexed_ea( f, addr, reg, 0 );
        break;

    case EAMODE_SUB_MODE:
        switch ( reg )
        {
        case 0:
        {
            ADDR dest = (ADDR)(WORD)nextw( f, addr );
            operand( "(" );
            emit_abs_addr( dest, FORMAT_IMM16, xtype );
            operand( ").W" );
            break;
        }
        case 1:
        {
            ADDR dest = (ADDR)read_s32( f, addr );
            operand( "(" );
            emit_abs_addr( dest, FORMAT_IMM32, xtype );
            operand( ").L" );
            break;
        }
        case 2:
        {
            ADDR extaddr = *addr;
            LWORD disp = read_s16( f, addr );
            ADDR dest = extaddr + disp;
            operand( "(" );
            operand( xref_genwordaddr( NULL, FORMAT_IMM32, dest ) );
            if ( xtype != X_NONE )
                xref_addxref( xtype, g_insn_addr, dest );
            operand( ",PC)" );
            break;
        }
        case 3:
            emit_indexed_ea( f, addr, 0, 1 );
            break;
        case 4:
            emit_imm_ea( f, addr, size );
            break;
        default:
            operand( "???" );
            break;
        }
        break;
    
    default:
        operand( "???" );
        break;
    
    }
}

static int ea_field_is_control( int ea )
{
    int mode = (ea >> 3) & 0x7;
    int reg  = ea & 0x7;

    return mode == EAMODE_ADDR_INDIR
        || mode == EAMODE_ADDR_IND_DISP
        || mode == EAMODE_ADDR_IND_IDX
        || (mode == EAMODE_SUB_MODE && reg <= 3);
}

static int ea_field_is_register( int ea )
{
    int mode = (ea >> 3) & 0x7;

    return mode == EAMODE_DATA_DIRECT || mode == EAMODE_ADDR_DIRECT;
}

static int ea_field_is_data( int ea )
{
    int mode = (ea >> 3) & 0x7;
    int reg  = ea & 0x7;

    return mode == EAMODE_DATA_DIRECT
        || mode == EAMODE_ADDR_INDIR
        || mode == EAMODE_ADDR_POST_INC
        || mode == EAMODE_ADDR_PRE_DEC
        || mode == EAMODE_ADDR_IND_DISP
        || mode == EAMODE_ADDR_IND_IDX
        || (mode == EAMODE_SUB_MODE && reg <= 4);
}

static int ea_field_is_memory_alterable( int ea )
{
    int mode = (ea >> 3) & 0x7;
    int reg  = ea & 0x7;

    return mode == EAMODE_ADDR_INDIR
        || mode == EAMODE_ADDR_POST_INC
        || mode == EAMODE_ADDR_PRE_DEC
        || mode == EAMODE_ADDR_IND_DISP
        || mode == EAMODE_ADDR_IND_IDX
        || (mode == EAMODE_SUB_MODE && reg <= 1);
}

static int ea_field_is_data_alterable( int ea )
{
    int mode = (ea >> 3) & 0x7;
    int reg  = ea & 0x7;

    return mode == EAMODE_DATA_DIRECT
        || mode == EAMODE_ADDR_INDIR
        || mode == EAMODE_ADDR_POST_INC
        || mode == EAMODE_ADDR_PRE_DEC
        || mode == EAMODE_ADDR_IND_DISP
        || mode == EAMODE_ADDR_IND_IDX
        || (mode == EAMODE_SUB_MODE && reg <= 1);
}

static int ea_field_is_alterable( int ea )
{
    int mode = (ea >> 3) & 0x7;
    int reg  = ea & 0x7;

    return mode == EAMODE_DATA_DIRECT
        || mode == EAMODE_ADDR_DIRECT
        || mode == EAMODE_ADDR_INDIR
        || mode == EAMODE_ADDR_POST_INC
        || mode == EAMODE_ADDR_PRE_DEC
        || mode == EAMODE_ADDR_IND_DISP
        || mode == EAMODE_ADDR_IND_IDX
        || (mode == EAMODE_SUB_MODE && reg <= 1);
}

static int size_from_bits_76( OPC opc )
{
    switch ( (opc >> 6) & 0x03 )
    {
    case 0:
        return OPSIZE_BYTE;
    case 1:
        return OPSIZE_WORD;
    case 2:
        return OPSIZE_LONG;
    default:
        return 0;
    }
}

static int move_size( OPC opc )
{
    switch ( (opc >> 12) & 0x03 )
    {
    case 1:
        return OPSIZE_BYTE;
    case 2:
        return OPSIZE_LONG;
    case 3:
        return OPSIZE_WORD;
    default:
        return 0;
    }
}

static const char *size_suffix( int size )
{
    switch ( size )
    {
    case OPSIZE_BYTE:
        return ".B";
    case OPSIZE_WORD:
        return ".W";
    case OPSIZE_LONG:
        return ".L";
    default:
        return "";
    }
}

static const char *condition_name( int cc )
{
    static const char *names[] = {
        "T", "F", "HI", "LS", "CC", "CS", "NE", "EQ",
        "VC", "VS", "PL", "MI", "GE", "LT", "GT", "LE"
    };

    return names[cc & 0x0F];
}

static const char *fpu_condition_name( int cc )
{
    static const char *names[] = {
        "F", "EQ", "OGT", "OGE", "OLT", "OLE", "OGL", "OR",
        "UN", "UEQ", "UGT", "UGE", "ULT", "ULE", "NE", "T",
        "SF", "SEQ", "GT", "GE", "LT", "LE", "GL", "GLE",
        "NGLE", "NGL", "NLE", "NLT", "NGE", "NGT", "SNE", "ST"
    };

    return names[cc & 0x1F];
}

static int fpu_size_from_format( int fmt )
{
    switch ( fmt & 0x07 )
    {
    case 0:
        return OPSIZE_LONG;
    case 1:
        return OPSIZE_LONG;
    case 2:
        return OPSIZE_EXTENDED;
    case 3:
        return OPSIZE_EXTENDED;
    case 4:
        return OPSIZE_WORD;
    case 5:
        return OPSIZE_DOUBLE;
    case 6:
        return OPSIZE_BYTE;
    case 7:
        return OPSIZE_EXTENDED;
    default:
        return 0;
    }
}

static int fpu_ea_is_source( int ea, int fmt )
{
    if ( ea_field_is_register( ea ) )
        return ((ea >> 3) & 0x07) == EAMODE_DATA_DIRECT
            && (fmt == 0 || fmt == 1 || fmt == 4 || fmt == 6);

    return ea_field_is_data( ea );
}

static int fpu_ea_is_destination( int ea, int fmt )
{
    if ( ea_field_is_register( ea ) )
        return ((ea >> 3) & 0x07) == EAMODE_DATA_DIRECT
            && (fmt == 0 || fmt == 1 || fmt == 4 || fmt == 6);

    return ea_field_is_memory_alterable( ea );
}

static int move_dest_ea( OPC opc )
{
    return ((opc >> 3) & 0x38) | ((opc >> 9) & 0x07);
}

static void emit_bad_operands( void )
{
    operand( "???" );
}

OPERAND_FUNC(ea)
{
    emit_ea_field( f, addr, opc & 0x3F, OPSIZE_WORD, xtype );
}

OPERAND_FUNC(ea_control)
{
    int ea = opc & 0x3F;

    if ( ea_field_is_control( ea ) )
        emit_ea_field( f, addr, ea, OPSIZE_LONG, xtype );
    else
        operand( "???" );
}

OPERAND_FUNC(move)
{
    int size = move_size( opc );
    int src = opc & 0x3F;
    int dst = move_dest_ea( opc );

    if ( size == 0 || !ea_field_is_data( src ) || !ea_field_is_data_alterable( dst ) )
    {
        emit_bad_operands();
        return;
    }

    emit_ea_field( f, addr, src, size, xtype );
    operand( ", " );
    emit_ea_field( f, addr, dst, size, X_NONE );
}

OPERAND_FUNC(movea)
{
    int size = move_size( opc );
    int src = opc & 0x3F;
    int dst = (opc >> 9) & 0x07;

    if ( size == OPSIZE_BYTE || !ea_field_is_data( src ) )
    {
        emit_bad_operands();
        return;
    }

    emit_ea_field( f, addr, src, size, xtype );
    operand( ", " FORMAT_AREG, dst );
}

OPERAND_FUNC(ea_dreg9_s76)
{
    int size = size_from_bits_76( opc );
    int ea = opc & 0x3F;

    if ( size == 0 || !ea_field_is_data( ea ) )
    {
        emit_bad_operands();
        return;
    }

    emit_ea_field( f, addr, ea, size, xtype );
    operand( ", " FORMAT_DREG, (opc >> 9) & 0x07 );
}

OPERAND_FUNC(dreg9_ea_s76)
{
    int size = size_from_bits_76( opc );
    int ea = opc & 0x3F;

    if ( size == 0 || !ea_field_is_data_alterable( ea ) )
    {
        emit_bad_operands();
        return;
    }

    operand( FORMAT_DREG ", ", (opc >> 9) & 0x07 );
    emit_ea_field( f, addr, ea, size, xtype );
}

OPERAND_FUNC(ea_areg9_s76)
{
    int size = size_from_bits_76( opc );
    int ea = opc & 0x3F;

    if ( size == OPSIZE_BYTE || !ea_field_is_data( ea ) )
    {
        emit_bad_operands();
        return;
    }

    emit_ea_field( f, addr, ea, size, xtype );
    operand( ", " FORMAT_AREG, (opc >> 9) & 0x07 );
}

OPERAND_FUNC(imm_ea_s76)
{
    int size = size_from_bits_76( opc );
    int ea = opc & 0x3F;

    if ( size == 0 || !ea_field_is_data_alterable( ea ) )
    {
        emit_bad_operands();
        return;
    }

    emit_imm_ea( f, addr, size );
    operand( ", " );
    emit_ea_field( f, addr, ea, size, xtype );
}

OPERAND_FUNC(bitnum_ea)
{
    int ea = opc & 0x3F;
    UWORD bitnum = nextw( f, addr );

    if ( !ea_field_is_data( ea ) || !ea_field_is_data_alterable( ea ) )
    {
        emit_bad_operands();
        return;
    }

    operand( "#" FORMAT_IMM8 ", ", bitnum & 0xFF );
    emit_ea_field( f, addr, ea, ea_field_is_register( ea ) ? OPSIZE_LONG : OPSIZE_BYTE, xtype );
}

OPERAND_FUNC(dreg9_ea_bit)
{
    int ea = opc & 0x3F;

    if ( !ea_field_is_data( ea ) )
    {
        emit_bad_operands();
        return;
    }

    operand( FORMAT_DREG ", ", (opc >> 9) & 0x07 );
    emit_ea_field( f, addr, ea, ea_field_is_register( ea ) ? OPSIZE_LONG : OPSIZE_BYTE, xtype );
}

OPERAND_FUNC(quick_ea_s76)
{
    int size = size_from_bits_76( opc );
    int ea = opc & 0x3F;
    int data = (opc >> 9) & 0x07;

    if ( data == 0 )
        data = 8;

    if ( size == 0 || !ea_field_is_alterable( ea ) )
    {
        emit_bad_operands();
        return;
    }

    operand( "#" FORMAT_IMM8 ", ", data );
    emit_ea_field( f, addr, ea, size, xtype );
}

OPERAND_FUNC(scc_ea)
{
    int ea = opc & 0x3F;

    if ( !ea_field_is_data_alterable( ea ) )
    {
        emit_bad_operands();
        return;
    }

    emit_ea_field( f, addr, ea, OPSIZE_BYTE, xtype );
}

OPERAND_FUNC(dbcc)
{
    WORD disp = (WORD)nextw( f, addr );
    ADDR dest = *addr + disp;

    operand( FORMAT_DREG ", ", opc & 0x07 );
    operand( xref_genwordaddr( NULL, FORMAT_IMM32, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

OPERAND_FUNC(ea_s76)
{
    int size = size_from_bits_76( opc );
    int ea = opc & 0x3F;

    if ( size == 0 || !ea_field_is_data_alterable( ea ) )
    {
        emit_bad_operands();
        return;
    }

    emit_ea_field( f, addr, ea, size, xtype );
}

OPERAND_FUNC(ea_word_dreg9)
{
    int ea = opc & 0x3F;

    if ( !ea_field_is_data( ea ) )
    {
        emit_bad_operands();
        return;
    }

    emit_ea_field( f, addr, ea, OPSIZE_WORD, xtype );
    operand( ", " FORMAT_DREG, (opc >> 9) & 0x07 );
}

OPERAND_FUNC(ea_long_areg9)
{
    int ea = opc & 0x3F;

    if ( !ea_field_is_control( ea ) )
    {
        emit_bad_operands();
        return;
    }

    emit_ea_field( f, addr, ea, OPSIZE_LONG, xtype );
    operand( ", " FORMAT_AREG, (opc >> 9) & 0x07 );
}

OPERAND_FUNC(usp_areg0)
{
    operand( "USP, " FORMAT_AREG, opc & 0x07 );
}

OPERAND_FUNC(areg0_usp)
{
    operand( FORMAT_AREG ", USP", opc & 0x07 );
}

OPERAND_FUNC(disp_areg0_dreg9)
{
    WORD disp = (WORD)nextw( f, addr );

    operand( "(" );
    emit_displacement( disp, FORMAT_IMM16 );
    operand( "," FORMAT_AREG "), " FORMAT_DREG, opc & 0x07, (opc >> 9) & 0x07 );
}

OPERAND_FUNC(dreg9_disp_areg0)
{
    WORD disp = (WORD)nextw( f, addr );

    operand( FORMAT_DREG ", (", (opc >> 9) & 0x07 );
    emit_displacement( disp, FORMAT_IMM16 );
    operand( "," FORMAT_AREG ")", opc & 0x07 );
}

static void emit_movem_reglist( UWORD mask, int predecrement )
{
    int bit;
    int need_comma = 0;

    for ( bit = 0; bit < 16; bit++ )
    {
        int regnum;
        char regtype;

        if ( predecrement )
        {
            if ( bit < 8 )
            {
                regtype = 'A';
                regnum = 7 - bit;
            }
            else
            {
                regtype = 'D';
                regnum = 15 - bit;
            }
        }
        else
        {
            if ( bit < 8 )
            {
                regtype = 'D';
                regnum = bit;
            }
            else
            {
                regtype = 'A';
                regnum = bit - 8;
            }
        }

        if ( mask & (1 << bit) )
        {
            operand( "%s%c%d", need_comma ? "/" : "", regtype, regnum );
            need_comma = 1;
        }
    }
}

OPERAND_FUNC(movem_regs_ea)
{
    UWORD regmask = nextw( f, addr );
    int ea = opc & 0x3F;
    int size = (opc & 0x0040) ? OPSIZE_LONG : OPSIZE_WORD;
    int predecrement = ((ea >> 3) & 0x07) == EAMODE_ADDR_PRE_DEC;

    if ( !ea_field_is_control( ea ) && !ea_field_is_memory_alterable( ea ) )
    {
        emit_bad_operands();
        return;
    }

    emit_movem_reglist( regmask, predecrement );
    operand( ", " );
    emit_ea_field( f, addr, ea, size, xtype );
}

OPERAND_FUNC(movem_ea_regs)
{
    UWORD regmask = nextw( f, addr );
    int ea = opc & 0x3F;
    int size = (opc & 0x0040) ? OPSIZE_LONG : OPSIZE_WORD;

    if ( !ea_field_is_control( ea ) && !ea_field_is_memory_alterable( ea ) )
    {
        emit_bad_operands();
        return;
    }

    emit_ea_field( f, addr, ea, size, xtype );
    operand( ", " );
    emit_movem_reglist( regmask, 0 );
}

OPERAND_FUNC(shift_reg)
{
    int count = (opc >> 9) & 0x07;
    int reg = opc & 0x07;

    if ( opc & 0x20 )
        operand( FORMAT_DREG ", " FORMAT_DREG, count, reg );
    else
    {
        if ( count == 0 )
            count = 8;
        operand( "#" FORMAT_IMM8 ", " FORMAT_DREG, count, reg );
    }
}

OPERAND_FUNC(shift_mem)
{
    int ea = opc & 0x3F;

    if ( !ea_field_is_memory_alterable( ea ) )
    {
        emit_bad_operands();
        return;
    }

    emit_ea_field( f, addr, ea, OPSIZE_WORD, xtype );
}

OPERAND_FUNC(imm8_ccr)
{
    operand( "#" FORMAT_IMM8 ", CCR", (UWORD)nextw( f, addr ) & 0xFF );
}

OPERAND_FUNC(imm16_sr)
{
    operand( "#" FORMAT_IMM16 ", SR", (UWORD)nextw( f, addr ) );
}

OPERAND_FUNC(sr_ea)
{
    int ea = opc & 0x3F;

    if ( !ea_field_is_data_alterable( ea ) )
    {
        emit_bad_operands();
        return;
    }

    operand( "SR, " );
    emit_ea_field( f, addr, ea, OPSIZE_WORD, xtype );
}

OPERAND_FUNC(ccr_ea)
{
    int ea = opc & 0x3F;

    if ( !ea_field_is_data_alterable( ea ) )
    {
        emit_bad_operands();
        return;
    }

    operand( "CCR, " );
    emit_ea_field( f, addr, ea, OPSIZE_WORD, xtype );
}

OPERAND_FUNC(ea_ccr)
{
    int ea = opc & 0x3F;

    if ( !ea_field_is_data( ea ) )
    {
        emit_bad_operands();
        return;
    }

    emit_ea_field( f, addr, ea, OPSIZE_WORD, xtype );
    operand( ", CCR" );
}

OPERAND_FUNC(ea_sr)
{
    int ea = opc & 0x3F;

    if ( !ea_field_is_data( ea ) )
    {
        emit_bad_operands();
        return;
    }

    emit_ea_field( f, addr, ea, OPSIZE_WORD, xtype );
    operand( ", SR" );
}

static const char *control_reg_name( UWORD ctrl )
{
    switch ( ctrl )
    {
    case 0x000:
        return "SFC";
    case 0x001:
        return "DFC";
    case 0x800:
        return "USP";
    case 0x801:
        return "VBR";
    case 0x002:
        return "CACR";
    case 0x802:
        return "CAAR";
    case 0x803:
        return "MSP";
    case 0x804:
        return "ISP";
    default:
        return NULL;
    }
}

static void emit_movec_reg( UWORD ext )
{
    operand( "%c%d", (ext & 0x8000) ? 'A' : 'D', (ext >> 12) & 0x07 );
}

static void emit_control_reg( UWORD ctrl )
{
    const char *name = control_reg_name( ctrl );

    if ( name )
        operand( "%s", name );
    else
        operand( "CR$%03X", ctrl );
}

OPERAND_FUNC(creg_reg)
{
    UWORD ext = nextw( f, addr );

    emit_control_reg( ext & 0x0FFF );
    operand( ", " );
    emit_movec_reg( ext );
}

OPERAND_FUNC(reg_creg)
{
    UWORD ext = nextw( f, addr );

    emit_movec_reg( ext );
    operand( ", " );
    emit_control_reg( ext & 0x0FFF );
}

OPERAND_FUNC(moves)
{
    UWORD ext = nextw( f, addr );
    int size = size_from_bits_76( opc );
    int ea = opc & 0x3F;
    char regtype = (ext & 0x8000) ? 'A' : 'D';
    int reg = (ext >> 12) & 0x07;

    if ( size == 0 || !ea_field_is_data_alterable( ea ) )
    {
        emit_bad_operands();
        return;
    }

    if ( ext & 0x0800 )
    {
        operand( "%c%d, ", regtype, reg );
        emit_ea_field( f, addr, ea, size, xtype );
    }
    else
    {
        emit_ea_field( f, addr, ea, size, xtype );
        operand( ", %c%d", regtype, reg );
    }
}

static void emit_bitfield_spec( UWORD ext )
{
    int offset = (ext >> 6) & 0x1F;
    int width = ext & 0x1F;

    operand( "{" );
    if ( ext & 0x0800 )
        operand( FORMAT_DREG, offset & 0x07 );
    else
        operand( "#" FORMAT_IMM8, offset );

    operand( ":" );
    if ( ext & 0x0020 )
        operand( FORMAT_DREG, width & 0x07 );
    else
        operand( "#" FORMAT_IMM8, width == 0 ? 32 : width );
    operand( "}" );
}

OPERAND_FUNC(bitfield_ea)
{
    UWORD ext = nextw( f, addr );
    int ea = opc & 0x3F;

    if ( !ea_field_is_data( ea ) )
    {
        emit_bad_operands();
        return;
    }

    emit_ea_field( f, addr, ea, OPSIZE_LONG, xtype );
    emit_bitfield_spec( ext );
}

OPERAND_FUNC(bitfield_ea_dreg)
{
    UWORD ext = nextw( f, addr );
    int ea = opc & 0x3F;

    if ( !ea_field_is_data( ea ) )
    {
        emit_bad_operands();
        return;
    }

    emit_ea_field( f, addr, ea, OPSIZE_LONG, xtype );
    emit_bitfield_spec( ext );
    operand( ", " FORMAT_DREG, (ext >> 12) & 0x07 );
}

OPERAND_FUNC(bitfield_dreg_ea)
{
    UWORD ext = nextw( f, addr );
    int ea = opc & 0x3F;

    if ( !ea_field_is_data_alterable( ea ) )
    {
        emit_bad_operands();
        return;
    }

    operand( FORMAT_DREG ", ", (ext >> 12) & 0x07 );
    emit_ea_field( f, addr, ea, OPSIZE_LONG, xtype );
    emit_bitfield_spec( ext );
}

OPERAND_FUNC(trapcc)
{
    /* empty */
}

OPERAND_FUNC(trapcc_imm16)
{
    operand( "#" FORMAT_IMM16, (UWORD)nextw( f, addr ) );
}

OPERAND_FUNC(trapcc_imm32)
{
    operand( "#" FORMAT_IMM32, (ULWORD)read_s32( f, addr ) );
}

OPERAND_FUNC(fbranch16)
{
    WORD disp = (WORD)nextw( f, addr );
    ADDR dest = *addr + disp;

    operand( xref_genwordaddr( NULL, FORMAT_IMM32, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

OPERAND_FUNC(fbranch32)
{
    LWORD disp = read_s32( f, addr );
    ADDR dest = *addr + disp;

    operand( xref_genwordaddr( NULL, FORMAT_IMM32, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

OPERAND_FUNC(fscc_ea)
{
    int ea = opc & 0x3F;

    nextw( f, addr );

    if ( !ea_field_is_data_alterable( ea ) )
    {
        emit_bad_operands();
        return;
    }

    emit_ea_field( f, addr, ea, OPSIZE_BYTE, xtype );
}

OPERAND_FUNC(fdbcc)
{
    WORD disp;
    ADDR dest;

    nextw( f, addr );
    disp = (WORD)nextw( f, addr );
    dest = *addr + disp;

    operand( FORMAT_DREG ", ", opc & 0x07 );
    operand( xref_genwordaddr( NULL, FORMAT_IMM32, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

OPERAND_FUNC(ftrapcc)
{
    nextw( f, addr );
}

OPERAND_FUNC(ftrapcc_imm16)
{
    nextw( f, addr );
    operand( "#" FORMAT_IMM16, (UWORD)nextw( f, addr ) );
}

OPERAND_FUNC(ftrapcc_imm32)
{
    nextw( f, addr );
    operand( "#" FORMAT_IMM32, (ULWORD)read_s32( f, addr ) );
}

OPERAND_FUNC(fpreg_fpreg)
{
    UWORD ext = nextw( f, addr );

    operand( "FP%d, FP%d", (ext >> 10) & 0x07, (ext >> 7) & 0x07 );
}

OPERAND_FUNC(fpreg)
{
    UWORD ext = nextw( f, addr );

    operand( "FP%d", (ext >> 10) & 0x07 );
}

OPERAND_FUNC(fpu_ea_fpreg)
{
    UWORD ext = nextw( f, addr );
    int fmt = (ext >> 10) & 0x07;
    int ea = opc & 0x3F;

    if ( !fpu_ea_is_source( ea, fmt ) )
    {
        emit_bad_operands();
        return;
    }

    emit_ea_field( f, addr, ea, fpu_size_from_format( fmt ), xtype );
    operand( ", FP%d", (ext >> 7) & 0x07 );
}

OPERAND_FUNC(fpu_fpreg_ea)
{
    UWORD ext = nextw( f, addr );
    int fmt = (ext >> 10) & 0x07;
    int ea = opc & 0x3F;

    if ( !fpu_ea_is_destination( ea, fmt ) )
    {
        emit_bad_operands();
        return;
    }

    operand( "FP%d, ", (ext >> 7) & 0x07 );
    emit_ea_field( f, addr, ea, fpu_size_from_format( fmt ), xtype );
}

OPERAND_FUNC(fmovecr)
{
    UWORD ext = nextw( f, addr );

    operand( "#" FORMAT_IMM8 ", FP%d", ext & 0x7F, (ext >> 7) & 0x07 );
}

/******************************************************************************/
/**                            Opcode Functions                              **/
/******************************************************************************/

static const char *opcode_move( OPC opc )
{
    static char text[16];

    sprintf( text, "MOVE%s", size_suffix( move_size( opc ) ) );
    return text;
}

static const char *opcode_movea( OPC opc )
{
    static char text[16];

    sprintf( text, "MOVEA%s", size_suffix( move_size( opc ) ) );
    return text;
}

static const char *opcode_size76( const char *base, OPC opc )
{
    static char text[16];

    sprintf( text, "%s%s", base, size_suffix( size_from_bits_76( opc ) ) );
    return text;
}

static const char *opcode_ori( OPC opc )  { return opcode_size76( "ORI",  opc ); }
static const char *opcode_andi( OPC opc ) { return opcode_size76( "ANDI", opc ); }
static const char *opcode_subi( OPC opc ) { return opcode_size76( "SUBI", opc ); }
static const char *opcode_addi( OPC opc ) { return opcode_size76( "ADDI", opc ); }
static const char *opcode_eori( OPC opc ) { return opcode_size76( "EORI", opc ); }
static const char *opcode_cmpi( OPC opc ) { return opcode_size76( "CMPI", opc ); }
static const char *opcode_negx( OPC opc ) { return opcode_size76( "NEGX", opc ); }
static const char *opcode_clr( OPC opc )  { return opcode_size76( "CLR",  opc ); }
static const char *opcode_neg( OPC opc )  { return opcode_size76( "NEG",  opc ); }
static const char *opcode_not( OPC opc )  { return opcode_size76( "NOT",  opc ); }
static const char *opcode_tst( OPC opc )  { return opcode_size76( "TST",  opc ); }
static const char *opcode_addq( OPC opc ) { return opcode_size76( "ADDQ", opc ); }
static const char *opcode_subq( OPC opc ) { return opcode_size76( "SUBQ", opc ); }
static const char *opcode_addx( OPC opc ) { return opcode_size76( "ADDX", opc ); }
static const char *opcode_subx( OPC opc ) { return opcode_size76( "SUBX", opc ); }

static const char *opcode_to_dreg9( const char *base, OPC opc )
{
    return opcode_size76( base, opc );
}

static const char *opcode_or_ea_dreg9( OPC opc )  { return opcode_to_dreg9( "OR",  opc ); }
static const char *opcode_or_dreg9_ea( OPC opc )  { return opcode_to_dreg9( "OR",  opc ); }
static const char *opcode_sub_ea_dreg9( OPC opc ) { return opcode_to_dreg9( "SUB", opc ); }
static const char *opcode_sub_dreg9_ea( OPC opc ) { return opcode_to_dreg9( "SUB", opc ); }
static const char *opcode_cmp( OPC opc )          { return opcode_to_dreg9( "CMP", opc ); }
static const char *opcode_eor( OPC opc )          { return opcode_to_dreg9( "EOR", opc ); }
static const char *opcode_and_ea_dreg9( OPC opc ) { return opcode_to_dreg9( "AND", opc ); }
static const char *opcode_and_dreg9_ea( OPC opc ) { return opcode_to_dreg9( "AND", opc ); }
static const char *opcode_add_ea_dreg9( OPC opc ) { return opcode_to_dreg9( "ADD", opc ); }
static const char *opcode_add_dreg9_ea( OPC opc ) { return opcode_to_dreg9( "ADD", opc ); }

static const char *opcode_scc( OPC opc )
{
    static char text[16];

    sprintf( text, "S%s", condition_name( (opc >> 8) & 0x0F ) );
    return text;
}

static const char *opcode_dbcc( OPC opc )
{
    static char text[16];

    sprintf( text, "DB%s", condition_name( (opc >> 8) & 0x0F ) );
    return text;
}

static const char *opcode_trapcc( OPC opc )
{
    static char text[16];

    sprintf( text, "TRAP%s", condition_name( (opc >> 8) & 0x0F ) );
    return text;
}

static const char *opcode_fbcc( OPC opc )
{
    static char text[16];

    sprintf( text, "FB%s", fpu_condition_name( opc & 0x1F ) );
    return text;
}

static const char *opcode_shift_reg( OPC opc )
{
    static char text[16];
    static const char *names[] = { "AS", "LS", "ROX", "RO" };
    int kind = (opc >> 3) & 0x03;
    int left = opc & 0x0100;

    sprintf( text, "%s%c%s", names[kind], left ? 'L' : 'R',
             size_suffix( size_from_bits_76( opc ) ) );
    return text;
}

static const char *opcode_shift_mem( OPC opc )
{
    static char text[16];
    static const char *names[] = { "AS", "LS", "ROX", "RO" };
    int kind = (opc >> 9) & 0x03;
    int left = opc & 0x0100;

    sprintf( text, "%s%c.W", names[kind], left ? 'L' : 'R' );
    return text;
}

#define MASK_DYN(M_opcode_fn, M_ops, M_mask, M_val, M_xt)  \
    { .type     = OPTAB_MASK,                              \
      .opcode   = "DYNAMIC",                               \
      .opcode_fn = opcode_ ## M_opcode_fn,                  \
      .operands = operand_ ## M_ops,                        \
      .xtype    = M_xt,                                     \
      .u.mask.mask = M_mask,                                \
      .u.mask.val  = M_val                                  \
    },

#define MASK_DYN_CPU(M_opcode_fn, M_ops, M_mask, M_val, M_xt, M_min_cpu) \
    { .type     = OPTAB_MASK,                                            \
      .min_cpu  = M_min_cpu,                                             \
      .opcode   = "DYNAMIC",                                             \
      .opcode_fn = opcode_ ## M_opcode_fn,                                \
      .operands = operand_ ## M_ops,                                      \
      .xtype    = M_xt,                                                   \
      .u.mask.mask = M_mask,                                              \
      .u.mask.val  = M_val                                                \
    },

#define MASK_DYN_FPU(M_opcode_fn, M_ops, M_mask, M_val, M_xt, M_min_fpu) \
    { .type     = OPTAB_MASK,                                            \
      .min_fpu  = M_min_fpu,                                             \
      .opcode   = "DYNAMIC",                                             \
      .opcode_fn = opcode_ ## M_opcode_fn,                                \
      .operands = operand_ ## M_ops,                                      \
      .xtype    = M_xt,                                                   \
      .u.mask.mask = M_mask,                                              \
      .u.mask.val  = M_val                                                \
    },

#define FPU_OP_X(M_opcode, M_code) \
    MASK_EXT_FPU ( M_opcode ".X", fpreg_fpreg, 0xFFFF, 0xF200, 0xE07F, M_code, X_REG, 68881 )

#define FPU_TEST_X(M_code) \
    MASK_EXT_FPU ( "FTST.X", fpreg, 0xFFFF, 0xF200, 0xE07F, M_code, X_REG, 68881 )

#define FPU_OP_EA_FMT(M_opcode, M_suffix, M_fmt, M_code) \
    MASK_EXT_FPU ( M_opcode M_suffix, fpu_ea_fpreg, 0xFFC0, 0xF200, 0xFC7F, 0x4000 | ((M_fmt) << 10) | (M_code), X_NONE, 68881 )

#define FPU_MOVE_EA_FMT(M_suffix, M_fmt) \
    FPU_OP_EA_FMT ( "FMOVE", M_suffix, M_fmt, 0x00 )

#define FPU_OP_EA(M_opcode, M_code) \
    FPU_OP_EA_FMT ( M_opcode, ".L", 0, M_code ) \
    FPU_OP_EA_FMT ( M_opcode, ".S", 1, M_code ) \
    FPU_OP_EA_FMT ( M_opcode, ".X", 2, M_code ) \
    FPU_OP_EA_FMT ( M_opcode, ".P", 3, M_code ) \
    FPU_OP_EA_FMT ( M_opcode, ".W", 4, M_code ) \
    FPU_OP_EA_FMT ( M_opcode, ".D", 5, M_code ) \
    FPU_OP_EA_FMT ( M_opcode, ".B", 6, M_code ) \
    FPU_OP_EA_FMT ( M_opcode, ".P", 7, M_code )

#define FPU_TEST_EA_FMT(M_suffix, M_fmt, M_code) \
    MASK_EXT_FPU ( "FTST" M_suffix, fpu_ea_fpreg, 0xFFC0, 0xF200, 0xFC7F, 0x4000 | ((M_fmt) << 10) | (M_code), X_NONE, 68881 )

#define FPU_TEST_EA(M_code) \
    FPU_TEST_EA_FMT ( ".L", 0, M_code ) \
    FPU_TEST_EA_FMT ( ".S", 1, M_code ) \
    FPU_TEST_EA_FMT ( ".X", 2, M_code ) \
    FPU_TEST_EA_FMT ( ".P", 3, M_code ) \
    FPU_TEST_EA_FMT ( ".W", 4, M_code ) \
    FPU_TEST_EA_FMT ( ".D", 5, M_code ) \
    FPU_TEST_EA_FMT ( ".B", 6, M_code ) \
    FPU_TEST_EA_FMT ( ".P", 7, M_code )

#define FPU_MOVE_FP_EA_FMT(M_suffix, M_fmt) \
    MASK_EXT_FPU ( "FMOVE" M_suffix, fpu_fpreg_ea, 0xFFC0, 0xF200, 0xFC7F, 0x6000 | ((M_fmt) << 10), X_NONE, 68881 )

#define FPU_MOVE_FP_EA \
    FPU_MOVE_FP_EA_FMT ( ".L", 0 ) \
    FPU_MOVE_FP_EA_FMT ( ".S", 1 ) \
    FPU_MOVE_FP_EA_FMT ( ".X", 2 ) \
    FPU_MOVE_FP_EA_FMT ( ".P", 3 ) \
    FPU_MOVE_FP_EA_FMT ( ".W", 4 ) \
    FPU_MOVE_FP_EA_FMT ( ".D", 5 ) \
    FPU_MOVE_FP_EA_FMT ( ".B", 6 ) \
    FPU_MOVE_FP_EA_FMT ( ".P", 7 )

#define FPU_COND_TABLE(M_entry) \
    M_entry ( "F",    0x00 ) \
    M_entry ( "EQ",   0x01 ) \
    M_entry ( "OGT",  0x02 ) \
    M_entry ( "OGE",  0x03 ) \
    M_entry ( "OLT",  0x04 ) \
    M_entry ( "OLE",  0x05 ) \
    M_entry ( "OGL",  0x06 ) \
    M_entry ( "OR",   0x07 ) \
    M_entry ( "UN",   0x08 ) \
    M_entry ( "UEQ",  0x09 ) \
    M_entry ( "UGT",  0x0A ) \
    M_entry ( "UGE",  0x0B ) \
    M_entry ( "ULT",  0x0C ) \
    M_entry ( "ULE",  0x0D ) \
    M_entry ( "NE",   0x0E ) \
    M_entry ( "T",    0x0F ) \
    M_entry ( "SF",   0x10 ) \
    M_entry ( "SEQ",  0x11 ) \
    M_entry ( "GT",   0x12 ) \
    M_entry ( "GE",   0x13 ) \
    M_entry ( "LT",   0x14 ) \
    M_entry ( "LE",   0x15 ) \
    M_entry ( "GL",   0x16 ) \
    M_entry ( "GLE",  0x17 ) \
    M_entry ( "NGLE", 0x18 ) \
    M_entry ( "NGL",  0x19 ) \
    M_entry ( "NLE",  0x1A ) \
    M_entry ( "NLT",  0x1B ) \
    M_entry ( "NGE",  0x1C ) \
    M_entry ( "NGT",  0x1D ) \
    M_entry ( "SNE",  0x1E ) \
    M_entry ( "ST",   0x1F )

#define FPU_COND_ENTRIES(M_suffix, M_cc) \
    MASK_EXT_FPU ( "FDB" M_suffix, fdbcc, 0xFFF8, 0xF248, 0xFFFF, M_cc, X_JMP, 68881 ) \
    MASK_EXT_FPU ( "FTRAP" M_suffix, ftrapcc, 0xFFFF, 0xF27C, 0xFFFF, M_cc, X_NONE, 68881 ) \
    MASK_EXT_FPU ( "FTRAP" M_suffix, ftrapcc_imm16, 0xFFFF, 0xF27A, 0xFFFF, M_cc, X_IMM, 68881 ) \
    MASK_EXT_FPU ( "FTRAP" M_suffix, ftrapcc_imm32, 0xFFFF, 0xF27B, 0xFFFF, M_cc, X_IMM, 68881 ) \
    MASK_EXT_FPU ( "FS" M_suffix, fscc_ea, 0xFFC0, 0xF240, 0xFFFF, M_cc, X_NONE, 68881 )






















#if 0

/***********************************************************
 * Process "indexed" operand.
 * This mode uses a postbyte to determine the addressing
 * mode and how many additional instruction bytes to consume.
 ************************************************************/
 
enum {
    MODE_AUTO_INC  = 0x00,
    MODE_AUTO_INC2 = 0x01,
    MODE_AUTO_DEC  = 0x02,
    MODE_AUTO_DEC2 = 0x03,
    MODE_REG_ONLY  = 0x04,
    MODE_REG_ACCB  = 0x05,
    MODE_REG_ACCA  = 0x06,
    MODE_REG_8OFF  = 0x08,
    MODE_REG_16OFF = 0x09,
    MODE_REG_D     = 0x0B,
    MODE_PCR_8OFF  = 0x0C,
    MODE_PCR_16OFF = 0x0D,
    MODE_EXT_IND   = 0x0F    
};

OPERAND_FUNC(indexed)
{
    UBYTE postbyte = next( f, addr );
    UBYTE rr = ( postbyte >> 5 ) & 0x03;
    static const char * rrtab[] = { "X", "Y", "U", "S" };
    
    if ( postbyte & BIT(7) )
    {
        BYTE offset = ((BYTE)( ( postbyte & 0x1F ) << 3 )) >> 3;
        
        operand( "%d, %s", offset, rrtab[rr] );    
    }
    else
    {
        UBYTE mode = postbyte & 0x0F;
        int   ind  = postbyte & 0x10;
        
        if ( ind )
            operand("[");
            
        switch ( mode )
        {
        case MODE_AUTO_INC:
            operand( ",%s+", rrtab[rr] );
            break;
            
        case MODE_AUTO_INC2:
            operand( ",%s++", rrtab[rr] );
            break;
            
        case MODE_AUTO_DEC:
            operand( ",-%s", rrtab[rr] );
            break;
            
        case MODE_AUTO_DEC2:
            operand( ",--%s", rrtab[rr] );
            break;
            
        case MODE_REG_ONLY:
            operand( ",%s", rrtab[rr] );
            break;
            
        case MODE_REG_ACCB:
            operand( "B, %s", rrtab[rr] );
            break;
            
        case MODE_REG_ACCA:
            operand( "A, %s", rrtab[rr] );
            break;
            
        case MODE_REG_D:
            operand( "D, %s", rrtab[rr] );
            break;
            
        case MODE_REG_8OFF:
            {
                BYTE offset = (BYTE)next( f, addr );
                operand( "%d, %s", offset, rrtab[rr] );
            }
            break;
            
        case MODE_REG_16OFF:
            {
                UBYTE msb    = next( f, addr );
                UBYTE lsb    = next( f, addr );
                WORD  offset = MK_WORD( lsb, msb );
                operand( "%d, %s", offset, rrtab[rr] );
            }
            break;
            
        case MODE_PCR_8OFF:
            {
                BYTE offset = (BYTE)next( f, addr );
                operand( "%d, PCR", offset );
            }
            break;
            
        case MODE_PCR_16OFF:
            {
                UBYTE msb    = next( f, addr );
                UBYTE lsb    = next( f, addr );
                WORD  offset = MK_WORD( lsb, msb );
                operand( "%d, PCR", offset );
            }
            break;
            
        case MODE_EXT_IND:
            {
                UBYTE msb = next( f, addr );
                UBYTE lsb = next( f, addr );
                WORD  ea  = MK_WORD( lsb, msb );
                operand( "%d", ea );
            }
            break;
            
        default:
            operand( "???" );
            break;
        }
        
        if ( ind )
            operand("]");
    }
}

/***********************************************************
 * Process "extended" operands.
 ************************************************************/
 
OPERAND_FUNC(extended)
{
    UBYTE msb    = next( f, addr );
    UBYTE lsb    = next( f, addr );
    UWORD addr16 = MK_WORD( lsb, msb );

    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr16 ) );
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
    UBYTE msb = next( f, addr );
    UBYTE lsb = next( f, addr );
    WORD disp = MK_WORD( lsb, msb );
    ADDR dest = *addr + disp;
    
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

#endif
















/******************************************************************************/
/** Synthesized operands                                                     **/
/******************************************************************************/

TWO_OPERAND(areg0, simm16)
TWO_OPERAND(areg0, simm32)

TWO_OPERAND(dreg9, dreg0)
TWO_OPERAND(areg9, areg0)
TWO_OPERAND(dreg9, areg0)

TWO_OPERAND(dreg9, simm8)

/******************************************************************************/
/** Instruction Decoding Tables                                              **/
/** Note: tables are here as they refer to operand functions defined above.  **/
/******************************************************************************/

optab_t base_optab[] = {
    
/*----------------------------------------------------------------------------
  0000 - BIT, MOVEP, Imm
  ----------------------------------------------------------------------------*/

    INSN ( "ORI",       imm8_ccr,       0x003C,         X_IMM )
    INSN ( "ORI",       imm16_sr,       0x007C,         X_IMM )
    INSN ( "ANDI",      imm8_ccr,       0x023C,         X_IMM )
    INSN ( "ANDI",      imm16_sr,       0x027C,         X_IMM )
    INSN ( "EORI",      imm8_ccr,       0x0A3C,         X_IMM )
    INSN ( "EORI",      imm16_sr,       0x0A7C,         X_IMM )

    MASK ( "BTST",      bitnum_ea,      0xFFC0, 0x0800, X_IMM )
    MASK ( "BCHG",      bitnum_ea,      0xFFC0, 0x0840, X_IMM )
    MASK ( "BCLR",      bitnum_ea,      0xFFC0, 0x0880, X_IMM )
    MASK ( "BSET",      bitnum_ea,      0xFFC0, 0x08C0, X_IMM )

    MASK ( "MOVEP.W",   disp_areg0_dreg9, 0xF1F8, 0x0108, X_PTR )
    MASK ( "MOVEP.L",   disp_areg0_dreg9, 0xF1F8, 0x0148, X_PTR )
    MASK ( "MOVEP.W",   dreg9_disp_areg0, 0xF1F8, 0x0188, X_PTR )
    MASK ( "MOVEP.L",   dreg9_disp_areg0, 0xF1F8, 0x01C8, X_PTR )

    MASK ( "BTST",      dreg9_ea_bit,   0xF1C0, 0x0100, X_REG )
    MASK ( "BCHG",      dreg9_ea_bit,   0xF1C0, 0x0140, X_REG )
    MASK ( "BCLR",      dreg9_ea_bit,   0xF1C0, 0x0180, X_REG )
    MASK ( "BSET",      dreg9_ea_bit,   0xF1C0, 0x01C0, X_REG )

    MASK_DYN ( ori,     imm_ea_s76,     0xFFC0, 0x0000, X_IMM )
    MASK_DYN ( ori,     imm_ea_s76,     0xFFC0, 0x0040, X_IMM )
    MASK_DYN ( ori,     imm_ea_s76,     0xFFC0, 0x0080, X_IMM )
    MASK_DYN ( andi,    imm_ea_s76,     0xFFC0, 0x0200, X_IMM )
    MASK_DYN ( andi,    imm_ea_s76,     0xFFC0, 0x0240, X_IMM )
    MASK_DYN ( andi,    imm_ea_s76,     0xFFC0, 0x0280, X_IMM )
    MASK_DYN ( subi,    imm_ea_s76,     0xFFC0, 0x0400, X_IMM )
    MASK_DYN ( subi,    imm_ea_s76,     0xFFC0, 0x0440, X_IMM )
    MASK_DYN ( subi,    imm_ea_s76,     0xFFC0, 0x0480, X_IMM )
    MASK_DYN ( addi,    imm_ea_s76,     0xFFC0, 0x0600, X_IMM )
    MASK_DYN ( addi,    imm_ea_s76,     0xFFC0, 0x0640, X_IMM )
    MASK_DYN ( addi,    imm_ea_s76,     0xFFC0, 0x0680, X_IMM )
    MASK_DYN ( eori,    imm_ea_s76,     0xFFC0, 0x0A00, X_IMM )
    MASK_DYN ( eori,    imm_ea_s76,     0xFFC0, 0x0A40, X_IMM )
    MASK_DYN ( eori,    imm_ea_s76,     0xFFC0, 0x0A80, X_IMM )
    MASK_DYN ( cmpi,    imm_ea_s76,     0xFFC0, 0x0C00, X_IMM )
    MASK_DYN ( cmpi,    imm_ea_s76,     0xFFC0, 0x0C40, X_IMM )
    MASK_DYN ( cmpi,    imm_ea_s76,     0xFFC0, 0x0C80, X_IMM )

    MASK_CPU ( "MOVES.B", moves,        0xFFC0, 0x0E00, X_PTR, 68010 )
    MASK_CPU ( "MOVES.W", moves,        0xFFC0, 0x0E40, X_PTR, 68010 )
    MASK_CPU ( "MOVES.L", moves,        0xFFC0, 0x0E80, X_PTR, 68010 )


/*----------------------------------------------------------------------------
  0001 - MOVE byte
  ----------------------------------------------------------------------------*/

    MASK_DYN ( move,    move,           0xF000, 0x1000, X_PTR )

/*----------------------------------------------------------------------------
  0010 - MOVE long (32 bit)
  ----------------------------------------------------------------------------*/

    MASK_DYN ( movea,   movea,          0xF1C0, 0x2040, X_PTR )
    MASK_DYN ( move,    move,           0xF000, 0x2000, X_PTR )

/*----------------------------------------------------------------------------
  0011 - MOVE word (16 bit)
  ----------------------------------------------------------------------------*/

    MASK_DYN ( movea,   movea,          0xF1C0, 0x3040, X_PTR )
    MASK_DYN ( move,    move,           0xF000, 0x3000, X_PTR )




/*----------------------------------------------------------------------------
  0100 - MISC
  ----------------------------------------------------------------------------*/

    INSN ( "ILLEGAL",   none,           0x4AFC,         X_NONE )

    MASK ( "MOVE",      sr_ea,          0xFFC0, 0x40C0, X_REG )
    MASK_CPU ( "MOVE",  ccr_ea,         0xFFC0, 0x42C0, X_REG, 68010 )
    MASK ( "MOVE",      ea_ccr,         0xFFC0, 0x44C0, X_REG )
    MASK ( "MOVE",      ea_sr,          0xFFC0, 0x46C0, X_REG )

    MASK_DYN ( negx,    ea_s76,         0xFFC0, 0x4000, X_NONE )
    MASK_DYN ( negx,    ea_s76,         0xFFC0, 0x4040, X_NONE )
    MASK_DYN ( negx,    ea_s76,         0xFFC0, 0x4080, X_NONE )
    MASK_DYN ( clr,     ea_s76,         0xFFC0, 0x4200, X_NONE )
    MASK_DYN ( clr,     ea_s76,         0xFFC0, 0x4240, X_NONE )
    MASK_DYN ( clr,     ea_s76,         0xFFC0, 0x4280, X_NONE )
    MASK_DYN ( neg,     ea_s76,         0xFFC0, 0x4400, X_NONE )
    MASK_DYN ( neg,     ea_s76,         0xFFC0, 0x4440, X_NONE )
    MASK_DYN ( neg,     ea_s76,         0xFFC0, 0x4480, X_NONE )
    MASK_DYN ( not,     ea_s76,         0xFFC0, 0x4600, X_NONE )
    MASK_DYN ( not,     ea_s76,         0xFFC0, 0x4640, X_NONE )
    MASK_DYN ( not,     ea_s76,         0xFFC0, 0x4680, X_NONE )
    MASK_DYN ( tst,     ea_s76,         0xFFC0, 0x4A00, X_NONE )
    MASK_DYN ( tst,     ea_s76,         0xFFC0, 0x4A40, X_NONE )
    MASK_DYN ( tst,     ea_s76,         0xFFC0, 0x4A80, X_NONE )

    MASK_CPU ( "LINK.L", areg0_simm32,  0xFFF8, 0x4808, X_REG, 68020 )
    MASK ( "NBCD",      ea,             0xFFC0, 0x4800, X_NONE )
    MASK ( "TAS",       ea,             0xFFC0, 0x4AC0, X_NONE )
    MASK ( "CHK.W",     ea_word_dreg9,  0xF1C0, 0x4180, X_NONE )





/*----------------------------------------------------------------------------
  0101 - ADDQ, SUBQ, Scc, DBcc, TRAPc
  ----------------------------------------------------------------------------*/

    MASK_DYN ( dbcc,    dbcc,           0xF0F8, 0x50C8, X_JMP )
    MASK_DYN_CPU ( trapcc, trapcc,       0xF0FF, 0x50FC, X_NONE, 68020 )
    MASK_DYN_CPU ( trapcc, trapcc_imm16, 0xF0FF, 0x50FA, X_IMM,  68020 )
    MASK_DYN_CPU ( trapcc, trapcc_imm32, 0xF0FF, 0x50FB, X_IMM,  68020 )
    MASK_DYN ( scc,     scc_ea,         0xF0C0, 0x50C0, X_NONE )
    MASK_DYN ( addq,    quick_ea_s76,   0xF1C0, 0x5000, X_IMM )
    MASK_DYN ( addq,    quick_ea_s76,   0xF1C0, 0x5040, X_IMM )
    MASK_DYN ( addq,    quick_ea_s76,   0xF1C0, 0x5080, X_IMM )
    MASK_DYN ( subq,    quick_ea_s76,   0xF1C0, 0x5100, X_IMM )
    MASK_DYN ( subq,    quick_ea_s76,   0xF1C0, 0x5140, X_IMM )
    MASK_DYN ( subq,    quick_ea_s76,   0xF1C0, 0x5180, X_IMM )




/*----------------------------------------------------------------------------
  0110 - Bcc/BSR/BRA
  ----------------------------------------------------------------------------*/


    MASK ( "BRA",       relX,   0xFF00, 0x6000, X_JMP )
    MASK ( "BSR",       relX,   0xFF00, 0x6100, X_CALL )
    MASK ( "BHI",       relX,   0xFF00, 0x6200, X_JMP )
    MASK ( "BLS",       relX,   0xFF00, 0x6300, X_JMP )
    MASK ( "BCC",       relX,   0xFF00, 0x6400, X_JMP )
    MASK ( "BCS",       relX,   0xFF00, 0x6500, X_JMP )
    MASK ( "BNE",       relX,   0xFF00, 0x6600, X_JMP )
    MASK ( "BEQ",       relX,   0xFF00, 0x6700, X_JMP )
    MASK ( "BVC",       relX,   0xFF00, 0x6800, X_JMP )
    MASK ( "BVS",       relX,   0xFF00, 0x6900, X_JMP )
    MASK ( "BPL",       relX,   0xFF00, 0x6A00, X_JMP )
    MASK ( "BMI",       relX,   0xFF00, 0x6B00, X_JMP )
    MASK ( "BGE",       relX,   0xFF00, 0x6C00, X_JMP )
    MASK ( "BLT",       relX,   0xFF00, 0x6D00, X_JMP )
    MASK ( "BGT",       relX,   0xFF00, 0x6E00, X_JMP )
    MASK ( "BLE",       relX,   0xFF00, 0x6F00, X_JMP )



/*----------------------------------------------------------------------------
  0111 - MOVEQ
  ----------------------------------------------------------------------------*/

    MASK ( "MOVEQ",     dreg9_simm8,    0xF100, 0x7000, X_IMM )

/*----------------------------------------------------------------------------
  1000 - OR, DIV, SBCD
  ----------------------------------------------------------------------------*/

    MASK ( "DIVU.W",    ea_word_dreg9,  0xF1C0, 0x80C0, X_NONE )
    MASK ( "DIVS.W",    ea_word_dreg9,  0xF1C0, 0x81C0, X_NONE )
    MASK ( "SBCD",      dreg0_dreg9,    0xF1F8, 0x8100, X_REG )
    MASK ( "SBCD",      predec0_predec9, 0xF1F8, 0x8108, X_REG )

    MASK_DYN ( or_ea_dreg9, ea_dreg9_s76, 0xF1C0, 0x8000, X_NONE )
    MASK_DYN ( or_ea_dreg9, ea_dreg9_s76, 0xF1C0, 0x8040, X_NONE )
    MASK_DYN ( or_ea_dreg9, ea_dreg9_s76, 0xF1C0, 0x8080, X_NONE )
    MASK_DYN ( or_dreg9_ea, dreg9_ea_s76, 0xF1C0, 0x8100, X_NONE )
    MASK_DYN ( or_dreg9_ea, dreg9_ea_s76, 0xF1C0, 0x8140, X_NONE )
    MASK_DYN ( or_dreg9_ea, dreg9_ea_s76, 0xF1C0, 0x8180, X_NONE )




/*----------------------------------------------------------------------------
  1001 - SUB, SUBX
  ----------------------------------------------------------------------------*/

    MASK ( "SUBA.W",    ea_areg9_s76,   0xF1C0, 0x90C0, X_NONE )
    MASK ( "SUBA.L",    ea_areg9_s76,   0xF1C0, 0x91C0, X_NONE )
    MASK_DYN ( subx,    dreg0_dreg9,    0xF1F8, 0x9100, X_REG )
    MASK_DYN ( subx,    predec0_predec9, 0xF1F8, 0x9108, X_REG )
    MASK_DYN ( subx,    dreg0_dreg9,    0xF1F8, 0x9140, X_REG )
    MASK_DYN ( subx,    predec0_predec9, 0xF1F8, 0x9148, X_REG )
    MASK_DYN ( subx,    dreg0_dreg9,    0xF1F8, 0x9180, X_REG )
    MASK_DYN ( subx,    predec0_predec9, 0xF1F8, 0x9188, X_REG )

    MASK_DYN ( sub_ea_dreg9, ea_dreg9_s76, 0xF1C0, 0x9000, X_NONE )
    MASK_DYN ( sub_ea_dreg9, ea_dreg9_s76, 0xF1C0, 0x9040, X_NONE )
    MASK_DYN ( sub_ea_dreg9, ea_dreg9_s76, 0xF1C0, 0x9080, X_NONE )
    MASK_DYN ( sub_dreg9_ea, dreg9_ea_s76, 0xF1C0, 0x9100, X_NONE )
    MASK_DYN ( sub_dreg9_ea, dreg9_ea_s76, 0xF1C0, 0x9140, X_NONE )
    MASK_DYN ( sub_dreg9_ea, dreg9_ea_s76, 0xF1C0, 0x9180, X_NONE )



/*----------------------------------------------------------------------------
  1011 - CMP, EOR
  ----------------------------------------------------------------------------*/

    MASK ( "CMPA.W",    ea_areg9_s76,   0xF1C0, 0xB0C0, X_NONE )
    MASK ( "CMPA.L",    ea_areg9_s76,   0xF1C0, 0xB1C0, X_NONE )
    MASK ( "CMPM.B",    postinc0_postinc9, 0xF1F8, 0xB108, X_REG )
    MASK ( "CMPM.W",    postinc0_postinc9, 0xF1F8, 0xB148, X_REG )
    MASK ( "CMPM.L",    postinc0_postinc9, 0xF1F8, 0xB188, X_REG )

    MASK_DYN ( cmp,     ea_dreg9_s76,   0xF1C0, 0xB000, X_NONE )
    MASK_DYN ( cmp,     ea_dreg9_s76,   0xF1C0, 0xB040, X_NONE )
    MASK_DYN ( cmp,     ea_dreg9_s76,   0xF1C0, 0xB080, X_NONE )
    MASK_DYN ( eor,     dreg9_ea_s76,   0xF1C0, 0xB100, X_NONE )
    MASK_DYN ( eor,     dreg9_ea_s76,   0xF1C0, 0xB140, X_NONE )
    MASK_DYN ( eor,     dreg9_ea_s76,   0xF1C0, 0xB180, X_NONE )




/*----------------------------------------------------------------------------
  1100 - AND, MUL, ABCD, EXG
  ----------------------------------------------------------------------------*/

    MASK ( "MULU.W",    ea_word_dreg9,  0xF1C0, 0xC0C0, X_NONE )
    MASK ( "MULS.W",    ea_word_dreg9,  0xF1C0, 0xC1C0, X_NONE )
    MASK ( "ABCD",      dreg0_dreg9,    0xF1F8, 0xC100, X_REG )
    MASK ( "ABCD",      predec0_predec9, 0xF1F8, 0xC108, X_REG )

    MASK ( "EXG",       dreg9_dreg0,    0xF1F8, 0xC140, X_REG )
    MASK ( "EXG",       areg9_areg0,    0xF1F8, 0xC148, X_REG )
    MASK ( "EXG",       dreg9_areg0,    0xF1F8, 0xC188, X_REG )

    MASK_DYN ( and_ea_dreg9, ea_dreg9_s76, 0xF1C0, 0xC000, X_NONE )
    MASK_DYN ( and_ea_dreg9, ea_dreg9_s76, 0xF1C0, 0xC040, X_NONE )
    MASK_DYN ( and_ea_dreg9, ea_dreg9_s76, 0xF1C0, 0xC080, X_NONE )
    MASK_DYN ( and_dreg9_ea, dreg9_ea_s76, 0xF1C0, 0xC100, X_NONE )
    MASK_DYN ( and_dreg9_ea, dreg9_ea_s76, 0xF1C0, 0xC140, X_NONE )
    MASK_DYN ( and_dreg9_ea, dreg9_ea_s76, 0xF1C0, 0xC180, X_NONE )


/*----------------------------------------------------------------------------
  1101 - ADD, ADDX
  ----------------------------------------------------------------------------*/

    MASK ( "ADDA.W",    ea_areg9_s76,   0xF1C0, 0xD0C0, X_NONE )
    MASK ( "ADDA.L",    ea_areg9_s76,   0xF1C0, 0xD1C0, X_NONE )
    MASK_DYN ( addx,    dreg0_dreg9,    0xF1F8, 0xD100, X_REG )
    MASK_DYN ( addx,    predec0_predec9, 0xF1F8, 0xD108, X_REG )
    MASK_DYN ( addx,    dreg0_dreg9,    0xF1F8, 0xD140, X_REG )
    MASK_DYN ( addx,    predec0_predec9, 0xF1F8, 0xD148, X_REG )
    MASK_DYN ( addx,    dreg0_dreg9,    0xF1F8, 0xD180, X_REG )
    MASK_DYN ( addx,    predec0_predec9, 0xF1F8, 0xD188, X_REG )

    MASK_DYN ( add_ea_dreg9, ea_dreg9_s76, 0xF1C0, 0xD000, X_NONE )
    MASK_DYN ( add_ea_dreg9, ea_dreg9_s76, 0xF1C0, 0xD040, X_NONE )
    MASK_DYN ( add_ea_dreg9, ea_dreg9_s76, 0xF1C0, 0xD080, X_NONE )
    MASK_DYN ( add_dreg9_ea, dreg9_ea_s76, 0xF1C0, 0xD100, X_NONE )
    MASK_DYN ( add_dreg9_ea, dreg9_ea_s76, 0xF1C0, 0xD140, X_NONE )
    MASK_DYN ( add_dreg9_ea, dreg9_ea_s76, 0xF1C0, 0xD180, X_NONE )
    
    
    
    
/*----------------------------------------------------------------------------
  1110 - Shift, Rotate, Bit Field
  ----------------------------------------------------------------------------*/

    MASK_DYN ( shift_mem, shift_mem,     0xF8C0, 0xE0C0, X_NONE )
    MASK_CPU ( "BFTST",   bitfield_ea,      0xFFC0, 0xE8C0, X_NONE, 68020 )
    MASK_CPU ( "BFEXTU",  bitfield_ea_dreg, 0xFFC0, 0xE9C0, X_NONE, 68020 )
    MASK_CPU ( "BFCHG",   bitfield_ea,      0xFFC0, 0xEAC0, X_NONE, 68020 )
    MASK_CPU ( "BFEXTS",  bitfield_ea_dreg, 0xFFC0, 0xEBC0, X_NONE, 68020 )
    MASK_CPU ( "BFCLR",   bitfield_ea,      0xFFC0, 0xECC0, X_NONE, 68020 )
    MASK_CPU ( "BFFFO",   bitfield_ea_dreg, 0xFFC0, 0xEDC0, X_NONE, 68020 )
    MASK_CPU ( "BFSET",   bitfield_ea,      0xFFC0, 0xEEC0, X_NONE, 68020 )
    MASK_CPU ( "BFINS",   bitfield_dreg_ea, 0xFFC0, 0xEFC0, X_NONE, 68020 )
    MASK_DYN ( shift_reg, shift_reg,     0xF000, 0xE000, X_NONE )

/*----------------------------------------------------------------------------
  1111 - Coprocessor
  ----------------------------------------------------------------------------*/

    FPU_COND_TABLE ( FPU_COND_ENTRIES )

    MASK_DYN_FPU ( fbcc, fbranch16,      0xFFC0, 0xF280, X_JMP, 68881 )
    MASK_DYN_FPU ( fbcc, fbranch32,      0xFFC0, 0xF2C0, X_JMP, 68881 )
    MASK_EXT_FPU ( "FMOVECR.X", fmovecr, 0xFFFF, 0xF200, 0xFC00, 0x5C00, X_IMM, 68881 )

    FPU_OP_X ( "FMOVE",   0x00 )
    FPU_OP_X ( "FINT",    0x01 )
    FPU_OP_X ( "FSINH",   0x02 )
    FPU_OP_X ( "FINTRZ",  0x03 )
    FPU_OP_X ( "FSQRT",   0x04 )
    FPU_OP_X ( "FLOGNP1", 0x06 )
    FPU_OP_X ( "FETOXM1", 0x08 )
    FPU_OP_X ( "FTANH",   0x09 )
    FPU_OP_X ( "FATAN",   0x0A )
    FPU_OP_X ( "FASIN",   0x0C )
    FPU_OP_X ( "FATANH",  0x0D )
    FPU_OP_X ( "FSIN",    0x0E )
    FPU_OP_X ( "FTAN",    0x0F )
    FPU_OP_X ( "FETOX",   0x10 )
    FPU_OP_X ( "FTWOTOX", 0x11 )
    FPU_OP_X ( "FTENTOX", 0x12 )
    FPU_OP_X ( "FLOGN",   0x14 )
    FPU_OP_X ( "FLOG10",  0x15 )
    FPU_OP_X ( "FLOG2",   0x16 )
    FPU_OP_X ( "FABS",    0x18 )
    FPU_OP_X ( "FCOSH",   0x19 )
    FPU_OP_X ( "FNEG",    0x1A )
    FPU_OP_X ( "FACOS",   0x1C )
    FPU_OP_X ( "FCOS",    0x1D )
    FPU_OP_X ( "FGETEXP", 0x1E )
    FPU_OP_X ( "FGETMAN", 0x1F )
    FPU_OP_X ( "FDIV",    0x20 )
    FPU_OP_X ( "FMOD",    0x21 )
    FPU_OP_X ( "FADD",    0x22 )
    FPU_OP_X ( "FMUL",    0x23 )
    FPU_OP_X ( "FSGLDIV", 0x24 )
    FPU_OP_X ( "FREM",    0x25 )
    FPU_OP_X ( "FSCALE",  0x26 )
    FPU_OP_X ( "FSGLMUL", 0x27 )
    FPU_OP_X ( "FSUB",    0x28 )
    FPU_OP_X ( "FCMP",    0x38 )
    FPU_TEST_X ( 0x3A )

    FPU_OP_EA ( "FMOVE",   0x00 )
    FPU_OP_EA ( "FINT",    0x01 )
    FPU_OP_EA ( "FSINH",   0x02 )
    FPU_OP_EA ( "FINTRZ",  0x03 )
    FPU_OP_EA ( "FSQRT",   0x04 )
    FPU_OP_EA ( "FLOGNP1", 0x06 )
    FPU_OP_EA ( "FETOXM1", 0x08 )
    FPU_OP_EA ( "FTANH",   0x09 )
    FPU_OP_EA ( "FATAN",   0x0A )
    FPU_OP_EA ( "FASIN",   0x0C )
    FPU_OP_EA ( "FATANH",  0x0D )
    FPU_OP_EA ( "FSIN",    0x0E )
    FPU_OP_EA ( "FTAN",    0x0F )
    FPU_OP_EA ( "FETOX",   0x10 )
    FPU_OP_EA ( "FTWOTOX", 0x11 )
    FPU_OP_EA ( "FTENTOX", 0x12 )
    FPU_OP_EA ( "FLOGN",   0x14 )
    FPU_OP_EA ( "FLOG10",  0x15 )
    FPU_OP_EA ( "FLOG2",   0x16 )
    FPU_OP_EA ( "FABS",    0x18 )
    FPU_OP_EA ( "FCOSH",   0x19 )
    FPU_OP_EA ( "FNEG",    0x1A )
    FPU_OP_EA ( "FACOS",   0x1C )
    FPU_OP_EA ( "FCOS",    0x1D )
    FPU_OP_EA ( "FGETEXP", 0x1E )
    FPU_OP_EA ( "FGETMAN", 0x1F )
    FPU_OP_EA ( "FDIV",    0x20 )
    FPU_OP_EA ( "FMOD",    0x21 )
    FPU_OP_EA ( "FADD",    0x22 )
    FPU_OP_EA ( "FMUL",    0x23 )
    FPU_OP_EA ( "FSGLDIV", 0x24 )
    FPU_OP_EA ( "FREM",    0x25 )
    FPU_OP_EA ( "FSCALE",  0x26 )
    FPU_OP_EA ( "FSGLMUL", 0x27 )
    FPU_OP_EA ( "FSUB",    0x28 )
    FPU_OP_EA ( "FCMP",    0x38 )
    FPU_TEST_EA ( 0x3A )

    FPU_MOVE_FP_EA
    MASK_FPU ( "FRESTORE", ea_control,   0xFFC0, 0xF300, X_NONE, 68881 )
    MASK_FPU ( "FSAVE",    ea_control,   0xFFC0, 0xF340, X_NONE, 68881 )
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
  
    MASK ( "UNLK",      areg0,          0xFFF8, 0x4E58, X_REG )
    
    MASK ( "LINK",      areg0_simm16,   0xFFF8, 0x4E50, X_REG )
    
    
    
    
    

    MASK ( "EXT.W",     dreg0,  0xFFF8, 0x4880, X_REG )
    MASK ( "EXT.L",     dreg0,  0xFFF8, 0x48C0, X_REG )
    MASK_CPU ( "EXTB.L", dreg0, 0xFFF8, 0x49C0, X_REG, 68020 )

  
    MASK ( "SWAP",      dreg0, 0xFFF8, 0x4840, X_REG )
    MASK_CPU ( "BKPT",  vector3, 0xFFF8, 0x4848, X_NONE, 68010 )
    MASK ( "PEA",       ea_control,     0xFFC0, 0x4840, X_NONE )
    MASK ( "LEA",       ea_long_areg9,  0xF1C0, 0x41C0, X_PTR )
    MASK ( "MOVEM.W",   movem_regs_ea,  0xFFC0, 0x4880, X_PTR )
    MASK ( "MOVEM.L",   movem_regs_ea,  0xFFC0, 0x48C0, X_PTR )
    MASK ( "MOVEM.W",   movem_ea_regs,  0xFFC0, 0x4C80, X_PTR )
    MASK ( "MOVEM.L",   movem_ea_regs,  0xFFC0, 0x4CC0, X_PTR )

  
  
    INSN ( "NOP",       none,   0x4E71,         X_NONE )
    
    INSN ( "RTR",       none,   0x4E77,         X_NONE )
    INSN ( "RTS",       none,   0x4E75,         X_NONE )
    INSN_CPU ( "RTD",   imm16,  0x4E74,         X_IMM,  68010 )
    INSN_CPU ( "MOVEC", creg_reg, 0x4E7A,       X_REG,  68010 )
    INSN_CPU ( "MOVEC", reg_creg, 0x4E7B,       X_REG,  68010 )
    MASK ( "JSR",       ea_control,     0xFFC0, 0x4E80, X_CALL )
    MASK ( "JMP",       ea_control,     0xFFC0, 0x4EC0, X_JMP )

    MASK ( "MOVE",      usp_areg0,      0xFFF8, 0x4E60, X_REG )
    MASK ( "MOVE",      areg0_usp,      0xFFF8, 0x4E68, X_REG )

    INSN ( "RESET",     none,   0x4E70,         X_NONE )
    INSN ( "RTE",       none,   0x4E73,         X_NONE )
    
    INSN ( "STOP",      imm16,  0x4E72,         X_IMM  )
    
    MASK ( "TRAP",      vector4, 0xFFF0, 0x4E40, X_NONE )
    
    INSN ( "TRAPV",     none,   0x4E76,         X_NONE )


    INSN_CPU ( "PFLUSHA", none,  0xF518,         X_NONE, 68030 )



















    
#if 0

/*----------------------------------------------------------------------------
  Data Movement
  ----------------------------------------------------------------------------*/

LEA
MOVE
MOVEM
MOVEP
PEA

/*----------------------------------------------------------------------------
  Integer Arithmetic
  ----------------------------------------------------------------------------*/

ADD
ADDA
ADDI
ADDQ
ADDX
CLR
CMP
CMPA
CMPI
CMPM
CMP2
DIVS
DIVU
DIVSL
DIVUL
MULS
MULU
NEG
NEGX
SUB
SUBA
SUBI
SUBQ
SUBX

/*----------------------------------------------------------------------------
  Logical
  ----------------------------------------------------------------------------*/

AND
ANDI
EOR
EORI
NOT
OR
ORI

/*----------------------------------------------------------------------------
  Shift and Rotate
  ----------------------------------------------------------------------------*/

ASL
ASR
LSL
LSR
ROL
ROR
ROXL
ROXR


/*----------------------------------------------------------------------------
  Bit Manipulation
  ----------------------------------------------------------------------------*/

BCHG
BCLR
BSET
BTST

/*----------------------------------------------------------------------------
  Bit Field
  ----------------------------------------------------------------------------*/

BFCHG 
BFCLR 
BFEXTS
BFEXTU
BFFFO 
BFINS 
BFSET 
BFTST

/*----------------------------------------------------------------------------
  Binary-Coded Decimal
  ----------------------------------------------------------------------------*/

ABCD
NBCD
PACK
SBCD
UNPK

/*----------------------------------------------------------------------------
  Program Control
  ----------------------------------------------------------------------------*/

DBcc
Scc
BSR
JMP
JSR
RTD
TST
FTST

/*----------------------------------------------------------------------------
  System Control
  ----------------------------------------------------------------------------*/

ANDI to SR
EORI to SR
MOVE to SR
MOVE from SR
MOVE USP
MOVEC
MOVES
ORI to SR
CHK
CHK2 
TRAPcc
ANDI to SR
EORI to SR
MOVE to SR
MOVE from SR

/*----------------------------------------------------------------------------
  Multiprocessor
  ----------------------------------------------------------------------------*/

CAS
CAS2
TAS
cpBcc
cpDBcc
cpGEN
cpRESTORE
cpSAVE
cpScc
cpTRAPcc

/*----------------------------------------------------------------------------
  MMU
  ----------------------------------------------------------------------------*/

PFLUSH
PLOAD 
PMOVE 
PTEST

#endif

/*----------------------------------------------------------------------------*/
    
    END
};

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
