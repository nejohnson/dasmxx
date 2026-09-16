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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "dasmxx.h"
#include "optab.h"

/*****************************************************************************
 * Globally-visible decoder properties
 *****************************************************************************/

DASM_PROFILE( "dasmx86", "Intel x86", 8, 9, 0, 1, 1 )

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/

/* Common output formats */
#define FORMAT_NUM_8BIT      "0%02X"
#define FORMAT_NUM_16BIT     "0%04X"

/* Construct a 16-bit word out of low and high bytes */
#define MK_WORD(l,h)         ( ((l) & 0xFF) | (((h) & 0xFF) << 8) )

#define NOSEGPFX    ( 0 )
#define EMIT_SEG_PFX \
    if (segpfx) {operand("%s:", segreg[segpfx]); segpfx = NOSEGPFX;}

/*****************************************************************************
 * Private data.  Declare as static.
 *****************************************************************************/

static const char * const segreg[5]  = { "", "ES", "CS", "SS", "DS" };
static const char * const wordreg[8] = { "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI" };
static const char * const bytereg[8] = { "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH" };
static const char * const dwordreg[8]= { "EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI" };
static const char * const eareg[8]   = { "BX + SI", "BX + DI", "BP + SI", "BP + DI",
                                         "SI", "DI", "BP", "BX" };

static int segpfx = NOSEGPFX;

/*****************************************************************************
 *        Private Functions
 *****************************************************************************/

/******************************************************************************/
/**                            Prefix Functions                              **/
/******************************************************************************/

/*
 * Segment override prefix is of the form
 *        001_SS_110
 */
PREFIX_FUNC(pfx_seg)
{
    int reg = (opc >> 3) & 0x03;
    
    segpfx = reg + 1;
}

PREFIX_FUNC(pfx_rep)
{
    if ( opc & 1 )
        operand( "REP  " );
    else
        operand( "REPNZ " );
}

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

/* This operand just gobbles up the next byte with no effect */
OPERAND_FUNC(gobble)
{
    UBYTE unused = next( f, addr );
}

/******************************************************************************/
/**                            Single Operands                               **/
/******************************************************************************/

OPERAND_FUNC(dx)
{
    operand( "DX" );
}

OPERAND_FUNC(AX)
{
    operand( "AX" );
}

OPERAND_FUNC(reg)
{
    int reg = opc & 0x07;
    int isword = opc & 0x08;
    
    operand( (isword ? wordreg : bytereg)[reg] );
}

OPERAND_FUNC(reg16)
{
    int reg = opc & 0x07;
    
    operand( wordreg[reg] );
}

OPERAND_FUNC(reg32)
{
    int reg = opc & 0x07;

    operand( dwordreg[reg] );
}

OPERAND_FUNC(acc)
{
    int wordop = opc & 1;
    operand( wordop ? "AX" : "AL" );
}

OPERAND_FUNC(imm8)
{
    UBYTE byte = next( f, addr );
    
    operand( FORMAT_NUM_8BIT, byte );
}

OPERAND_FUNC(port8)
{
    UBYTE byte = next( f, addr );
    
    operand( FORMAT_NUM_8BIT, byte );
}

/*
 * Near (intra-segment) branch targets are computed modulo 64K: the 8086
 * family adds the displacement to IP, which is 16 bits wide, and the
 * segment base is unchanged.  dasmxx works in a flat address space, so a
 * plain "*addr + disp" escapes the segment on any backwards branch taken
 * from near the bottom of the address space (e.g. E9 F0 FF at 0x00100 must
 * target 0x000F3, not 0x100F3).  NEAR_TARGET converts the flat address back
 * to an offset within the segment named by the command file's 'g' command,
 * wraps the addition to 16 bits, and converts back.  With no 'g' command
 * the segment base is 0, which reproduces plain 8086 (CS=0) behaviour.
 */
#define NEAR_TARGET(pc, disp)  \
    ( dasm_segment_base + ( ( (pc) - dasm_segment_base + (disp) ) & 0xFFFF ) )

OPERAND_FUNC(disp8)
{
    BYTE disp = (BYTE)next( f, addr );
    ADDR dest = NEAR_TARGET( *addr, (ADDR)(LWORD)disp );

    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

OPERAND_FUNC(imm16)
{
    UBYTE lsb   = next( f, addr );
    UBYTE msb   = next( f, addr );
    UWORD imm16 = MK_WORD( lsb, msb );

    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, imm16 ) );
    xref_addxref( xtype, g_insn_addr, imm16 );
}

OPERAND_FUNC(addr16)
{
    UBYTE lsb = next( f, addr );
    UBYTE msb = next( f, addr );
    ADDR dest = MK_WORD( lsb, msb );
    
    EMIT_SEG_PFX;
    operand( "%c[%s]", 
        opc & 1 ? 'W' : 'B', 
        xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

OPERAND_FUNC(disp16)
{
    UBYTE lsb = next( f, addr );
    UBYTE msb = next( f, addr );
    ADDR dest = NEAR_TARGET( *addr, (ADDR)MK_WORD( lsb, msb ) );

    EMIT_SEG_PFX;
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

OPERAND_FUNC(segoff)
{
    UBYTE offlo = next( f, addr );
    UBYTE offhi = next( f, addr );
    UBYTE seglo = next( f, addr );
    UBYTE seghi = next( f, addr );

    ADDR offset = MK_WORD( offlo, offhi );
    ADDR segment = MK_WORD( seglo, seghi );
    ADDR linear  = ( segment << 4 ) + offset;
    char *label  = xref_findaddrlabel( linear );

    /*
     * Far pointers are printed as SEG:OFF (the form that appears in the
     * ROM image) with the resolved flat address, and any label already
     * attached to it, alongside - and the flat address is registered as a
     * cross-reference so far call/jump targets show up in the -x dump.
     */
    if ( label )
        operand( FORMAT_NUM_16BIT ":" FORMAT_NUM_16BIT " {%s}", segment, offset, label );
    else
        operand( FORMAT_NUM_16BIT ":" FORMAT_NUM_16BIT " {%05X}", segment, offset, linear );

    xref_addxref( xtype, g_insn_addr, linear );
}

OPERAND_FUNC(segmreg)
{
    UBYTE reg = ( opc >> 3 ) & 3;
    
    operand( segreg[reg+1] );
}

OPERAND_FUNC(modrm)
{
    UBYTE arg  = next( f, addr );
    int mod    = (arg >> 6) & 3;
    int reg    = (arg >> 3) & 7;
    int rm     = arg & 7;
    int wordop = opc & 1;
    int dir    = opc & 2;
    int isseg  = 0;
    int dest, src, action;
    enum { DO_REG, DO_ADDR };
    
    /* Handle special-case opcodes */
    switch( opc )
    {
        case 0xC4: /* LES */
            wordop = 1;
            dir = 1;
            break;
            
        case 0x8D: /* LEA */
        case 0xC5: /* LDS */
            dir = 1;
            break;

        case 0x62: /* BOUND r16,m16&16      (80186) */
        case 0x69: /* IMUL r16,r/m16,imm16  (80186) */
        case 0x6B: /* IMUL r16,r/m16,imm8   (80186) */
            wordop = 1;
            dir = 1;
            break;

        case 0x8E:  /* MOV to SEG */
        case 0x8C:  /* MOV from SEG */
            isseg  = 1;
            wordop = 1;
            break;
    }
    
    /* ------------------ */

    /* Some opcodes use the REG field to encode the instruction selection.  In
     * these cases there is no REG, just the ADDR.
     * */
    if ( opc == 0xFF || opc == 0xFE || opc == 0x8F || opc == 0xD0 || opc == 0xD1
         || opc == 0xD2 || opc == 0xD3
         || opc == 0x80 || opc == 0x81 || opc == 0x83
         || opc == 0xC6 || opc == 0xC7
         || opc == 0xF6 || opc == 0xF7
         || opc == 0xC0 || opc == 0xC1 /* 80186 shift/rotate-by-imm8 group */
         || ((opc & 0xF8) == 0xD8) )
    {
        src = action = DO_ADDR;
    }
    else
    {
        dest = dir ? DO_REG : DO_ADDR;
        src  = dir ? DO_ADDR : DO_REG;
        action = dest;
    }
        
    do {
        switch ( action )
        {
        case DO_REG:
            if ( isseg )
                /* Only REG 0-3 name a segment register; 4-7 are undefined
                 * encodings of 8C/8E and used to index segreg[] out of
                 * bounds, passing a wild pointer to operand() -> SIGSEGV
                 * as soon as a data byte pair like "8C 34" was decoded. */
                operand( reg < 4 ? segreg[reg+1] : "?SEG?" );
            else
                operand( (wordop ? wordreg : bytereg)[reg] );
            break;
                
        case DO_ADDR:
            switch( mod )
            {
                case 0: /* MOD = 00, DISP is 0, except if rm = 110 then 
                         *  EA is 16-bit DISP */
                    EMIT_SEG_PFX;
                    if ( rm == 6 )
                    {
                        UBYTE displo = next( f, addr );
                        UBYTE disphi = next( f, addr );
                        ADDR disp = MK_WORD( displo, disphi );
                        operand( FORMAT_NUM_16BIT, disp );
                    }
                    else
                    {
                        operand( "%c[%s]", wordop ? 'W' : 'B', eareg[rm] );
                    }                
                    break;
                    
                case 1: /* MOD = 01, DISP is 8-bit sign-extended */
                {
                    BYTE disp = (BYTE)next( f, addr );
                    EMIT_SEG_PFX;
                    operand( "%c[%s + " FORMAT_NUM_8BIT "]", 
                        wordop ? 'W' : 'B',
                        eareg[rm], 
                        disp );
                    break;
                }
                    
                case 2: /* MOD = 10, DISP is 16-bit signed */
                {
                    UBYTE displo = next( f, addr );
                    UBYTE disphi = next( f, addr );
                    ADDR disp = MK_WORD( displo, disphi );
                    EMIT_SEG_PFX;
                    operand( "%c[%s + " FORMAT_NUM_16BIT "]", 
                        wordop ? 'W' : 'B',
                        eareg[rm], 
                        disp );                
                    break;
                }
                    
                case 3: /* MOD = 11, r/m is treated as reg field */
                    operand( (wordop ? wordreg : bytereg)[rm] );                
                    break;
            }
            break;
        }
        
        if ( action == src )
            break;
        action = src;
        operand( ", " );
    }
    while ( 1 );
}

OPERAND_FUNC(modrmC)
{
    UBYTE clreg = opc & 2;
    
    operand_modrm( f, addr, opc, xtype );
    
    if ( clreg )
        operand( ", CL" );
    else
        operand( ", 1" );    
}

OPERAND_FUNC(modrmimm)
{
    UBYTE datalo, datahi;
    UWORD imm16;
    
    operand_modrm( f, addr, opc, xtype );
    operand( ", " );
    
    /* Some variations do not support sign-extended immediates */
    if ( (opc & 0xFE) == 0xC6 || (opc & 0xFE) == 0xF6 )
        opc &= 1;
    
    switch( opc & 3 )
    {
    case 0: /* s:w = 00 :: 8-bit immediate */
        datalo = next( f, addr );
        operand( FORMAT_NUM_8BIT, datalo );
        break;
        
    case 1: /* s:w = 01 :: 16-bit immediate */
        datalo = next( f, addr );
        datahi = next( f, addr );
        imm16 = MK_WORD( datalo, datahi );
        operand( FORMAT_NUM_16BIT, imm16 );
        break;
        
    case 3: /* s:w = 11 :: 8-bit sign-extended to 16-bit */
        imm16 = next( f, addr );
        if ( imm16 & 0x80 ) imm16 |= 0xFF00;
        operand( FORMAT_NUM_16BIT, imm16 );
        break;
    }
}

/* 80186: IMUL r16,r/m16,imm16 (0x69) -- modrm() prints "REG, R/M" (dir
 * forced above), then the 16-bit immediate follows. */
OPERAND_FUNC(modrm_imm16)
{
    UBYTE lo, hi;
    UWORD imm16;

    operand_modrm( f, addr, opc, xtype );
    operand( ", " );

    lo = next( f, addr );
    hi = next( f, addr );
    imm16 = MK_WORD( lo, hi );
    operand( FORMAT_NUM_16BIT, imm16 );
}

/* 80186: IMUL r16,r/m16,imm8 (0x6B) -- immediate is sign-extended to 16
 * bits before the multiply, so print the sign-extended value to match the
 * modrm_imm16 sibling above. */
OPERAND_FUNC(modrm_imm8)
{
    UWORD imm16;

    operand_modrm( f, addr, opc, xtype );
    operand( ", " );

    imm16 = next( f, addr );
    if ( imm16 & 0x80 ) imm16 |= 0xFF00;
    operand( FORMAT_NUM_16BIT, imm16 );
}

/* 80186: shift/rotate-by-imm8 group (C0 /n ib, C1 /n ib) -- same REG-field
 * group selection as the D0-D3 shift/rotate-by-CL-or-1 group (modrmC
 * above), but the count is an explicit imm8 operand instead of "CL"/"1". */
OPERAND_FUNC(modrm_shiftimm)
{
    UBYTE count;

    operand_modrm( f, addr, opc, xtype );
    operand( ", " );

    count = next( f, addr );
    operand( FORMAT_NUM_8BIT, count );
}

static void operand_ea( FILE *f, ADDR *addr, UBYTE arg, const char *size, const char * const *regs )
{
    int mod = (arg >> 6) & 3;
    int rm  = arg & 7;

    switch( mod )
    {
    case 0:
        EMIT_SEG_PFX;
        if ( rm == 6 )
        {
            UBYTE displo = next( f, addr );
            UBYTE disphi = next( f, addr );
            ADDR disp = MK_WORD( displo, disphi );
            operand( "%s[" FORMAT_NUM_16BIT "]", size, disp );
        }
        else
        {
            operand( "%s[%s]", size, eareg[rm] );
        }
        break;

    case 1:
    {
        BYTE disp = (BYTE)next( f, addr );
        EMIT_SEG_PFX;
        operand( "%s[%s + " FORMAT_NUM_8BIT "]", size, eareg[rm], disp );
        break;
    }

    case 2:
    {
        UBYTE displo = next( f, addr );
        UBYTE disphi = next( f, addr );
        ADDR disp = MK_WORD( displo, disphi );
        EMIT_SEG_PFX;
        operand( "%s[%s + " FORMAT_NUM_16BIT "]", size, eareg[rm], disp );
        break;
    }

    case 3:
        operand( regs[rm] );
        break;
    }
}

static void operand_rm_reg( FILE *f, ADDR *addr, int wordop )
{
    UBYTE arg = next( f, addr );
    int reg = (arg >> 3) & 7;

    operand_ea( f, addr, arg, wordop ? "W" : "B", wordop ? wordreg : bytereg );
    COMMA;
    operand( (wordop ? wordreg : bytereg)[reg] );
}

static void operand_reg_rm( FILE *f, ADDR *addr, int dstword, int srcword )
{
    UBYTE arg = next( f, addr );
    int reg = (arg >> 3) & 7;

    operand( (dstword ? wordreg : bytereg)[reg] );
    COMMA;
    operand_ea( f, addr, arg, srcword ? "W" : "B", srcword ? wordreg : bytereg );
}

OPERAND_FUNC(rm16)
{
    UBYTE arg = next( f, addr );

    operand_ea( f, addr, arg, "W", wordreg );
}

OPERAND_FUNC(rm8)
{
    UBYTE arg = next( f, addr );

    operand_ea( f, addr, arg, "B", bytereg );
}

OPERAND_FUNC(reg16_rm16)
{
    operand_reg_rm( f, addr, 1, 1 );
}

OPERAND_FUNC(reg16_rm8)
{
    operand_reg_rm( f, addr, 1, 0 );
}

OPERAND_FUNC(rm16_reg16)
{
    operand_rm_reg( f, addr, 1 );
}

OPERAND_FUNC(rm8_reg8)
{
    operand_rm_reg( f, addr, 0 );
}

OPERAND_FUNC(rm16_reg16_imm8)
{
    operand_rm_reg( f, addr, 1 );
    COMMA;
    operand_imm8( f, addr, opc, xtype );
}

OPERAND_FUNC(rm16_reg16_CL)
{
    operand_rm_reg( f, addr, 1 );
    operand( ", CL" );
}

OPERAND_FUNC(rm16_imm8)
{
    UBYTE arg = next( f, addr );

    operand_ea( f, addr, arg, "W", wordreg );
    COMMA;
    operand_imm8( f, addr, opc, xtype );
}

/***********************************************************
 * 8087/80187 floating-point operands.
 ************************************************************/

static void operand_fp_ea( FILE *f, ADDR *addr, const char *size )
{
    UBYTE arg = next( f, addr );
    int mod   = (arg >> 6) & 3;
    int rm    = arg & 7;

    switch( mod )
    {
    case 0:
        EMIT_SEG_PFX;
        if ( rm == 6 )
        {
            UBYTE displo = next( f, addr );
            UBYTE disphi = next( f, addr );
            ADDR disp = MK_WORD( displo, disphi );
            operand( "%s[" FORMAT_NUM_16BIT "]", size, disp );
        }
        else
        {
            operand( "%s[%s]", size, eareg[rm] );
        }
        break;

    case 1:
    {
        BYTE disp = (BYTE)next( f, addr );
        EMIT_SEG_PFX;
        operand( "%s[%s + " FORMAT_NUM_8BIT "]", size, eareg[rm], disp );
        break;
    }

    case 2:
    {
        UBYTE displo = next( f, addr );
        UBYTE disphi = next( f, addr );
        ADDR disp = MK_WORD( displo, disphi );
        EMIT_SEG_PFX;
        operand( "%s[%s + " FORMAT_NUM_16BIT "]", size, eareg[rm], disp );
        break;
    }

    case 3:
        operand( "ST(%d)", rm );
        break;
    }
}

OPERAND_FUNC(fp_m16int)
{
    operand_fp_ea( f, addr, "W" );
}

OPERAND_FUNC(fp_m32int)
{
    operand_fp_ea( f, addr, "D" );
}

OPERAND_FUNC(fp_m64int)
{
    operand_fp_ea( f, addr, "Q" );
}

OPERAND_FUNC(fp_m32real)
{
    operand_fp_ea( f, addr, "D" );
}

OPERAND_FUNC(fp_m64real)
{
    operand_fp_ea( f, addr, "Q" );
}

OPERAND_FUNC(fp_m80real)
{
    operand_fp_ea( f, addr, "T" );
}

OPERAND_FUNC(fp_m80bcd)
{
    operand_fp_ea( f, addr, "T" );
}

OPERAND_FUNC(fp_menv)
{
    operand_fp_ea( f, addr, "ENV" );
}

OPERAND_FUNC(fp_m16)
{
    operand_fp_ea( f, addr, "W" );
}

OPERAND_FUNC(fp_sti)
{
    UBYTE arg = next( f, addr );

    operand( "ST(%d)", arg & 7 );
}

OPERAND_FUNC(fp_st_sti)
{
    UBYTE arg = next( f, addr );

    operand( "ST, ST(%d)", arg & 7 );
}

OPERAND_FUNC(fp_sti_st)
{
    UBYTE arg = next( f, addr );

    operand( "ST(%d), ST", arg & 7 );
}

OPERAND_FUNC(gobble_AX)
{
    UBYTE unused = next( f, addr );

    operand_AX( f, addr, opc, xtype );
}

/******************************************************************************/
/**                            Double Operands                               **/
/******************************************************************************/

TWO_OPERAND_PAIR(acc, addr16)
TWO_OPERAND_PAIR(acc, port8)
TWO_OPERAND_PAIR(acc, dx)

TWO_OPERAND(acc, imm8)
TWO_OPERAND(acc, imm16)

TWO_OPERAND(reg, imm8)
TWO_OPERAND(reg, imm16)
TWO_OPERAND(AX, reg16)

TWO_OPERAND(imm16, imm8)  /* 80186 ENTER: allocsize, nestlevel */

/******************************************************************************/
/** Instruction Decoding Tables                                              **/
/** Note: tables are here as they refer to operand functions defined above.  **/
/******************************************************************************/

static optab_t x86_0f_optab[] = {
    MASK2( "SLDT", rm16,        0x00, 0x38, 0x00, X_NONE )
    MASK2( "STR",  rm16,        0x00, 0x38, 0x08, X_NONE )
    MASK2( "LLDT", rm16,        0x00, 0x38, 0x10, X_NONE )
    MASK2( "LTR",  rm16,        0x00, 0x38, 0x18, X_NONE )
    MASK2( "VERR", rm16,        0x00, 0x38, 0x20, X_NONE )
    MASK2( "VERW", rm16,        0x00, 0x38, 0x28, X_NONE )

    MASK2( "SGDT", rm16,        0x01, 0x38, 0x00, X_NONE )
    MASK2( "SIDT", rm16,        0x01, 0x38, 0x08, X_NONE )
    MASK2( "LGDT", rm16,        0x01, 0x38, 0x10, X_NONE )
    MASK2( "LIDT", rm16,        0x01, 0x38, 0x18, X_NONE )
    MASK2( "SMSW", rm16,        0x01, 0x38, 0x20, X_NONE )
    MASK2( "LMSW", rm16,        0x01, 0x38, 0x30, X_NONE )
    MASK2_CPU( "INVLPG", rm16,  0x01, 0x38, 0x38, X_NONE, 80486 )

    INSN(  "LAR",  reg16_rm16,  0x02, X_NONE )
    INSN(  "LSL",  reg16_rm16,  0x03, X_NONE )
    INSN(  "CLTS", none,        0x06, X_NONE )
    INSN_CPU( "INVD",   none,   0x08, X_NONE, 80486 )
    INSN_CPU( "WBINVD", none,   0x09, X_NONE, 80486 )

    INSN_CPU( "SHLD",  rm16_reg16_imm8, 0xA4, X_NONE, 80386 )
    INSN_CPU( "SHLD",  rm16_reg16_CL,   0xA5, X_NONE, 80386 )
    INSN_CPU( "BT",    rm16_reg16,      0xA3, X_NONE, 80386 )
    INSN_CPU( "BTS",   rm16_reg16,      0xAB, X_NONE, 80386 )
    INSN_CPU( "SHRD",  rm16_reg16_imm8, 0xAC, X_NONE, 80386 )
    INSN_CPU( "SHRD",  rm16_reg16_CL,   0xAD, X_NONE, 80386 )
    INSN_CPU( "BTR",   rm16_reg16,      0xB3, X_NONE, 80386 )
    INSN_CPU( "BTC",   rm16_reg16,      0xBB, X_NONE, 80386 )
    INSN_CPU( "BSF",   reg16_rm16,      0xBC, X_NONE, 80386 )
    INSN_CPU( "BSR",   reg16_rm16,      0xBD, X_NONE, 80386 )
    INSN_CPU( "MOVZX", reg16_rm8,       0xB6, X_NONE, 80386 )
    INSN_CPU( "MOVZX", reg16_rm16,      0xB7, X_NONE, 80386 )
    INSN_CPU( "MOVSX", reg16_rm8,       0xBE, X_NONE, 80386 )
    INSN_CPU( "MOVSX", reg16_rm16,      0xBF, X_NONE, 80386 )
    INSN_CPU( "CMPXCHG", rm8_reg8,      0xB0, X_NONE, 80486 )
    INSN_CPU( "CMPXCHG", rm16_reg16,    0xB1, X_NONE, 80486 )
    INSN_CPU( "XADD",    rm8_reg8,      0xC0, X_NONE, 80486 )
    INSN_CPU( "XADD",    rm16_reg16,    0xC1, X_NONE, 80486 )
    MASK_CPU( "BSWAP",   reg32,         0xF8, 0xC8, X_NONE, 80486 )

    MASK2_CPU( "BT",  rm16_imm8, 0xBA, 0x38, 0x20, X_NONE, 80386 )
    MASK2_CPU( "BTS", rm16_imm8, 0xBA, 0x38, 0x28, X_NONE, 80386 )
    MASK2_CPU( "BTR", rm16_imm8, 0xBA, 0x38, 0x30, X_NONE, 80386 )
    MASK2_CPU( "BTC", rm16_imm8, 0xBA, 0x38, 0x38, X_NONE, 80386 )

    INSN_CPU( "SETO",   rm8, 0x90, X_NONE, 80386 )
    INSN_CPU( "SETNO",  rm8, 0x91, X_NONE, 80386 )
    INSN_CPU( "SETB",   rm8, 0x92, X_NONE, 80386 )
    INSN_CPU( "SETNB",  rm8, 0x93, X_NONE, 80386 )
    INSN_CPU( "SETZ",   rm8, 0x94, X_NONE, 80386 )
    INSN_CPU( "SETNZ",  rm8, 0x95, X_NONE, 80386 )
    INSN_CPU( "SETBE",  rm8, 0x96, X_NONE, 80386 )
    INSN_CPU( "SETNBE", rm8, 0x97, X_NONE, 80386 )
    INSN_CPU( "SETS",   rm8, 0x98, X_NONE, 80386 )
    INSN_CPU( "SETNS",  rm8, 0x99, X_NONE, 80386 )
    INSN_CPU( "SETP",   rm8, 0x9A, X_NONE, 80386 )
    INSN_CPU( "SETNP",  rm8, 0x9B, X_NONE, 80386 )
    INSN_CPU( "SETL",   rm8, 0x9C, X_NONE, 80386 )
    INSN_CPU( "SETNL",  rm8, 0x9D, X_NONE, 80386 )
    INSN_CPU( "SETLE",  rm8, 0x9E, X_NONE, 80386 )
    INSN_CPU( "SETNLE", rm8, 0x9F, X_NONE, 80386 )

    END
};

optab_t base_optab[] = {

/* This has to go first because it is actually a pseudonym for "XCHG AX,AX" */
    INSN( "NOP",    none,  0x90, X_NONE )

/* In strict 8086 mode 0F falls through to the legacy POP CS match below. */
    TABLE_CPU( x86_0f_optab, 0x0F, 80286 )
    
/*----------------------------------------------------------------------------
  DATA TRANSFER
  ----------------------------------------------------------------------------*/
  
    MASK( "MOV",    acc_addr16,  0xFE, 0xA0, X_NONE )
    MASK( "MOV",    addr16_acc,  0xFE, 0xA2, X_NONE )
    
    MASK( "MOV",    reg_imm8,    0xF8, 0xB0, X_NONE )
    MASK( "MOV",    reg_imm16,   0xF8, 0xB8, X_NONE )
    
    MASK( "MOV",    modrm,       0xFC, 0x88, X_NONE )
    MASK( "MOV",    modrm,       0xFD, 0x8C, X_NONE ) /* seg regs */
    
    MASK( "MOV",    modrmimm,    0xFE, 0xC6, X_NONE )    
    
    MASK( "IN",     acc_port8,   0xFE, 0xE4, X_NONE )
    MASK( "IN",     acc_dx,      0xFE, 0xEC, X_NONE )
        
    MASK( "OUT",    port8_acc,   0xFE, 0xE6, X_NONE )
    MASK( "OUT",    dx_acc,      0xFE, 0xEE, X_NONE )
    
    MASK( "PUSH",   reg16,       0xF8, 0x50, X_NONE )
    MASK( "PUSH",   segmreg,     0xE7, 0x06, X_NONE )
    MASK2( "PUSH",  modrm,       0xFF, 0x38, 0x30, X_NONE )
    INSN( "PUSH",   imm16,       0x68, X_NONE ) /* 80186 */
    INSN( "PUSH",   imm8,        0x6A, X_NONE ) /* 80186 */
    INSN( "PUSHA",  none,        0x60, X_NONE ) /* 80186 */

    MASK( "POP",    reg16,       0xF8, 0x58, X_NONE )
    MASK( "POP",    segmreg,     0xE7, 0x07, X_NONE )
    MASK2( "POP",   modrm,       0x8F, 0x38, 0x00, X_NONE )
    INSN( "POPA",   none,        0x61, X_NONE ) /* 80186 */

    MASK( "XCHG",   AX_reg16,    0xF8, 0x90, X_NONE )
    MASK( "XCHG",   modrm,       0xFE, 0x86, X_NONE )

    INSN( "XLAT",   none,        0xD7, X_NONE )
    INSN( "LEA",    modrm,       0x8D, X_NONE )
    INSN( "LDS",    modrm,       0xC5, X_NONE )
    INSN( "LES",    modrm,       0xC4, X_NONE )
    INSN( "BOUND",  modrm,       0x62, X_NONE ) /* 80186 */
    INSN( "ARPL",   rm16_reg16,  0x63, X_NONE ) /* 80286 */
    
    INSN( "LAHF",   none, 0x9F, X_NONE )
    INSN( "SAHF",   none, 0x9E, X_NONE )
    INSN( "PUSHF",  none, 0x9C, X_NONE )
    INSN( "POPF",   none, 0x9D, X_NONE )
  
/*----------------------------------------------------------------------------
  ARITHMETIC
  ----------------------------------------------------------------------------*/

#define ARITH_IMM_ACC(M_name, M_mask) \
    INSN(M_name,acc_imm8,M_mask,X_NONE ) \
    INSN(M_name,acc_imm16,M_mask|1,X_NONE )

    ARITH_IMM_ACC("ADD", 0x04)
    ARITH_IMM_ACC("ADC", 0x14)
    ARITH_IMM_ACC("SUB", 0x2C)
    ARITH_IMM_ACC("SBB", 0x1C)
    ARITH_IMM_ACC("CMP", 0x3C)
    ARITH_IMM_ACC("AND", 0x24)
    ARITH_IMM_ACC("TEST",0xA8)
    ARITH_IMM_ACC("OR",  0x0C)
    ARITH_IMM_ACC("XOR", 0x34)
    
#define ARITH_IMMX_RM(M_name,M_mask) \
    MASK2(M_name,modrmimm,0x80,0x38,M_mask,X_NONE) \
    MASK2(M_name,modrmimm,0x81,0x38,M_mask,X_NONE) \
    MASK2(M_name,modrmimm,0x83,0x38,M_mask,X_NONE)
    
    /* The 80/81/83 group selects the operation with the REG field of the
     * modrm byte, so the mask value below is (reg << 3).  ADC is reg=2 and
     * so must be 0x10; it read 0x80, which lies outside the 0x38 mask and
     * therefore never matched, leaving "ADC r/m,imm" undecodable.  The
     * 0x83 (sign-extended imm8) form applies to all eight operations, AND,
     * OR and XOR included.
     */
    ARITH_IMMX_RM( "ADD", 0x00 )
    ARITH_IMMX_RM( "OR",  0x08 )
    ARITH_IMMX_RM( "ADC", 0x10 )
    ARITH_IMMX_RM( "SBB", 0x18 )
    ARITH_IMMX_RM( "AND", 0x20 )
    ARITH_IMMX_RM( "SUB", 0x28 )
    ARITH_IMMX_RM( "XOR", 0x30 )
    ARITH_IMMX_RM( "CMP", 0x38 )

    /* Group 3 (opcodes F6/F7) selects the operation with the modrm REG
     * field: /0 (and the /1 alias) TEST imm, /2 NOT, /3 NEG, /4 MUL,
     * /5 IMUL, /6 DIV, /7 IDIV.  The table used to match F6/F7 with ANY
     * reg field as "TEST r/m,imm", so MUL/DIV/NEG/NOT decoded as TEST and
     * swallowed the following 1-2 bytes as a non-existent immediate -- a
     * silent resync error.  The group-3 entries proper are down with the
     * arithmetic ops below.
     */
    MASK2( "TEST",  modrmimm, 0xF6, 0x38, 0x00, X_NONE )
    MASK2( "TEST",  modrmimm, 0xF7, 0x38, 0x00, X_NONE )
    MASK2( "TEST",  modrmimm, 0xF6, 0x38, 0x08, X_NONE )
    MASK2( "TEST",  modrmimm, 0xF7, 0x38, 0x08, X_NONE )
    
    MASK( "ADD",    modrm, 0xFC, 0x00, X_NONE )
    MASK( "ADC",    modrm, 0xFC, 0x10, X_NONE )
    MASK( "SUB",    modrm, 0xFC, 0x28, X_NONE )
    MASK( "SBB",    modrm, 0xFC, 0x18, X_NONE )
    MASK( "CMP",    modrm, 0xFC, 0x38, X_NONE )
    MASK( "AND",    modrm, 0xFC, 0x20, X_NONE )
    MASK( "TEST",   modrm, 0xFC, 0x84, X_NONE )
    MASK( "OR",     modrm, 0xFC, 0x08, X_NONE )
    MASK( "XOR",    modrm, 0xFC, 0x30, X_NONE )
    
    MASK( "INC",    reg16, 0xF8, 0x40, X_NONE )
    MASK2( "INC",   modrm, 0xFF, 0x38, 0x00, X_NONE )
    MASK2( "INC",   modrm, 0xFE, 0x38, 0x00, X_NONE )
    
    MASK( "DEC",    reg16, 0xF8, 0x48, X_NONE )
    MASK2( "DEC",   modrm, 0xFE, 0x38, 0x08, X_NONE )
    MASK2( "DEC",   modrm, 0xFF, 0x38, 0x08, X_NONE )
    
    MASK2( "NEG",   modrm, 0xF6, 0x38, 0x18, X_NONE )
    MASK2( "NEG",   modrm, 0xF7, 0x38, 0x18, X_NONE )
    
    MASK2( "MUL",   modrm, 0xF6, 0x38, 0x20, X_NONE )
    MASK2( "MUL",   modrm, 0xF7, 0x38, 0x20, X_NONE )
    
    MASK2( "IMUL",  modrm, 0xF6, 0x38, 0x28, X_NONE )
    MASK2( "IMUL",  modrm, 0xF7, 0x38, 0x28, X_NONE )
    INSN( "IMUL",   modrm_imm16, 0x69, X_NONE ) /* 80186 r16,r/m16,imm16 */
    INSN( "IMUL",   modrm_imm8,  0x6B, X_NONE ) /* 80186 r16,r/m16,imm8  */

    MASK2( "DIV",   modrm, 0xF6, 0x38, 0x30, X_NONE )
    MASK2( "DIV",   modrm, 0xF7, 0x38, 0x30, X_NONE )
    
    MASK2( "IDIV",  modrm, 0xF6, 0x38, 0x38, X_NONE )
    MASK2( "IDIV",  modrm, 0xF7, 0x38, 0x38, X_NONE )
    
    INSN( "AAA",    none, 0x37, X_NONE )
    INSN( "DAA",    none, 0x27, X_NONE )
    INSN( "AAS",    none, 0x3F, X_NONE )
    INSN( "DAS",    none, 0x2F, X_NONE )
    MASK2( "AAM",   gobble, 0xD4, 0xFF, 0x0A, X_NONE )
    MASK2( "AAD",   gobble, 0xD5, 0xFF, 0x0A, X_NONE )
    INSN( "CBW",    none, 0x98, X_NONE )
    INSN( "CWD",    none, 0x99, X_NONE )
    
/*----------------------------------------------------------------------------
  LOGIC
  ----------------------------------------------------------------------------*/
  
    MASK2( "NOT",   modrm, 0xF6, 0x38, 0x10, X_NONE )
    MASK2( "NOT",   modrm, 0xF7, 0x38, 0x10, X_NONE )
    
#define SHIFT_ROT_GRP(M_name,M_mask) \
    MASK2( M_name, modrmC, 0xD0, 0x38, M_mask, X_NONE ) \
    MASK2( M_name, modrmC, 0xD1, 0x38, M_mask, X_NONE ) \
    MASK2( M_name, modrmC, 0xD2, 0x38, M_mask, X_NONE ) \
    MASK2( M_name, modrmC, 0xD3, 0x38, M_mask, X_NONE )
    
    SHIFT_ROT_GRP( "SHL", 0x20 )
    SHIFT_ROT_GRP( "SHR", 0x28 )
    SHIFT_ROT_GRP( "SAR", 0x38 )
    SHIFT_ROT_GRP( "ROL", 0x00 )
    SHIFT_ROT_GRP( "ROR", 0x08 )
    SHIFT_ROT_GRP( "RCL", 0x10 )
    SHIFT_ROT_GRP( "RCR", 0x18 )

/* 80186: shift/rotate-by-imm8 (C0 /n ib, C1 /n ib) -- same REG-field group
 * selectors as SHIFT_ROT_GRP above, explicit imm8 count instead of CL/1. */
#define SHIFT_ROT_IMM_GRP(M_name,M_mask) \
    MASK2( M_name, modrm_shiftimm, 0xC0, 0x38, M_mask, X_NONE ) \
    MASK2( M_name, modrm_shiftimm, 0xC1, 0x38, M_mask, X_NONE )

    SHIFT_ROT_IMM_GRP( "SHL", 0x20 )
    SHIFT_ROT_IMM_GRP( "SHR", 0x28 )
    SHIFT_ROT_IMM_GRP( "SAR", 0x38 )
    SHIFT_ROT_IMM_GRP( "ROL", 0x00 )
    SHIFT_ROT_IMM_GRP( "ROR", 0x08 )
    SHIFT_ROT_IMM_GRP( "RCL", 0x10 )
    SHIFT_ROT_IMM_GRP( "RCR", 0x18 )

/*----------------------------------------------------------------------------
  STRING MANIPULATION
  ----------------------------------------------------------------------------*/

    PREFIX( pfx_rep, 0xF2 )
    PREFIX( pfx_rep, 0xF3 )
  
    INSN( "MOVSB", none, 0xA4, X_NONE )
    INSN( "MOVSW", none, 0xA5, X_NONE )
    
    INSN( "CMPSB", none, 0xA6, X_NONE )
    INSN( "CMPSW", none, 0xA7, X_NONE )
    
    INSN( "MOVSB", none, 0xA4, X_NONE )
    INSN( "MOVSW", none, 0xA5, X_NONE )
    
    INSN( "SCASB", none, 0xAE, X_NONE )
    INSN( "SCASW", none, 0xAF, X_NONE )
    
    INSN( "LODSB", none, 0xAC, X_NONE )
    INSN( "LODSW", none, 0xAD, X_NONE )
    
    INSN( "STOSB", none, 0xAA, X_NONE )
    INSN( "STOSW", none, 0xAB, X_NONE )

    INSN( "INSB",  none, 0x6C, X_NONE ) /* 80186 */
    INSN( "INSW",  none, 0x6D, X_NONE ) /* 80186 */
    INSN( "OUTSB", none, 0x6E, X_NONE ) /* 80186 */
    INSN( "OUTSW", none, 0x6F, X_NONE ) /* 80186 */

/*----------------------------------------------------------------------------
  CONTROL TRANSFER
  ----------------------------------------------------------------------------*/
  
    INSN( "CALL",  disp16, 0xE8, X_CALL )
    INSN( "CALL",  segoff, 0x9A, X_CALL )
    MASK2( "CALL", modrm, 0xFF, 0x38, 0x10, X_CALL )
    MASK2( "CALL", modrm, 0xFF, 0x38, 0x18, X_CALL )
  
    INSN( "JMP",   disp8,  0xEB, X_JMP )
    INSN( "JMP",   disp16, 0xE9, X_JMP )
    INSN( "JMP",   segoff, 0xEA, X_JMP )
    MASK2( "JMP",  modrm, 0xFF, 0x38, 0x20, X_JMP )
    MASK2( "JMP",  modrm, 0xFF, 0x38, 0x28, X_JMP )
  
    INSN( "RETN",  none,   0xC3, X_NONE )
    INSN( "RETN",  imm16,  0xC2, X_NONE )
    INSN( "RETF",  none,   0xCB, X_NONE )
    INSN( "RETF",  imm16,  0xCA, X_NONE )
    INSN( "ENTER", imm16_imm8, 0xC8, X_NONE ) /* 80186 */
    INSN( "LEAVE", none,   0xC9, X_NONE )     /* 80186 */

    INSN( "JO",    disp8,  0x70, X_JMP )
    INSN( "JNO",   disp8,  0x71, X_JMP )
    INSN( "JB",    disp8,  0x72, X_JMP )
    INSN( "JNB",   disp8,  0x73, X_JMP )
    INSN( "JE",    disp8,  0x74, X_JMP )
    INSN( "JNE",   disp8,  0x75, X_JMP )
    INSN( "JBE",   disp8,  0x76, X_JMP )
    INSN( "JNBE",  disp8,  0x77, X_JMP )
    INSN( "JS",    disp8,  0x78, X_JMP )
    INSN( "JNS",   disp8,  0x79, X_JMP )
    INSN( "JP",    disp8,  0x7A, X_JMP )
    INSN( "JNP",   disp8,  0x7B, X_JMP )
    INSN( "JL",    disp8,  0x7C, X_JMP )
    INSN( "JNL",   disp8,  0x7D, X_JMP )
    INSN( "JLE",   disp8,  0x7E, X_JMP )
    INSN( "JNLE",  disp8,  0x7F, X_JMP )
    
    INSN( "LOOP",  disp8,  0xE2, X_JMP )
    INSN( "LOOPNZ",disp8,  0xE0, X_JMP )
    INSN( "LOOPZ", disp8,  0xE1, X_JMP )
    INSN( "JCXZ",  disp8,  0xE3, X_JMP )

    INSN( "INT",   imm8,   0xCD, X_NONE )
    INSN( "INT3",  none,   0xCC, X_NONE )
    INSN( "INTO",  none,   0xCE, X_NONE )
    INSN( "IRET",  none,   0xCF, X_NONE )

/*----------------------------------------------------------------------------
  PROCESSOR CONTROL
  ----------------------------------------------------------------------------*/
  
    INSN( "CLC",    none,  0xF8, X_NONE )
    INSN( "CMC",    none,  0xF5, X_NONE )
    INSN( "STC",    none,  0xF9, X_NONE )
    
    INSN( "CLD",    none,  0xFC, X_NONE )
    INSN( "STD",    none,  0xFD, X_NONE )
    
    INSN( "CLI",    none,  0xFA, X_NONE )
    INSN( "STI",    none,  0xFB, X_NONE )
    
    INSN( "HLT",    none,  0xF4, X_NONE )
    INSN( "WAIT",   none,  0x9B, X_NONE )

/*----------------------------------------------------------------------------
  FLOATING POINT (8087/80187)
  ----------------------------------------------------------------------------*/

#define FP_MEM(M_name, M_ops, M_opc, M_reg) \
    MASK2( M_name, M_ops, M_opc, 0xF8, (0x00 | M_reg), X_NONE ) \
    MASK2( M_name, M_ops, M_opc, 0xF8, (0x40 | M_reg), X_NONE ) \
    MASK2( M_name, M_ops, M_opc, 0xF8, (0x80 | M_reg), X_NONE )

    MASK2( "FADD",   fp_st_sti,  0xD8, 0xF8, 0xC0, X_NONE )
    MASK2( "FMUL",   fp_st_sti,  0xD8, 0xF8, 0xC8, X_NONE )
    MASK2( "FCOM",   fp_sti,     0xD8, 0xF8, 0xD0, X_NONE )
    MASK2( "FCOMP",  fp_sti,     0xD8, 0xF8, 0xD8, X_NONE )
    MASK2( "FSUB",   fp_st_sti,  0xD8, 0xF8, 0xE0, X_NONE )
    MASK2( "FSUBR",  fp_st_sti,  0xD8, 0xF8, 0xE8, X_NONE )
    MASK2( "FDIV",   fp_st_sti,  0xD8, 0xF8, 0xF0, X_NONE )
    MASK2( "FDIVR",  fp_st_sti,  0xD8, 0xF8, 0xF8, X_NONE )

    FP_MEM( "FADD",  fp_m32real, 0xD8, 0x00 )
    FP_MEM( "FMUL",  fp_m32real, 0xD8, 0x08 )
    FP_MEM( "FCOM",  fp_m32real, 0xD8, 0x10 )
    FP_MEM( "FCOMP", fp_m32real, 0xD8, 0x18 )
    FP_MEM( "FSUB",  fp_m32real, 0xD8, 0x20 )
    FP_MEM( "FSUBR", fp_m32real, 0xD8, 0x28 )
    FP_MEM( "FDIV",  fp_m32real, 0xD8, 0x30 )
    FP_MEM( "FDIVR", fp_m32real, 0xD8, 0x38 )

    MASK2( "FLD",    fp_sti,     0xD9, 0xF8, 0xC0, X_NONE )
    MASK2( "FXCH",   fp_sti,     0xD9, 0xF8, 0xC8, X_NONE )
    MASK2( "FNOP",   gobble,     0xD9, 0xFF, 0xD0, X_NONE )
    MASK2( "FCHS",   gobble,     0xD9, 0xFF, 0xE0, X_NONE )
    MASK2( "FABS",   gobble,     0xD9, 0xFF, 0xE1, X_NONE )
    MASK2( "FTST",   gobble,     0xD9, 0xFF, 0xE4, X_NONE )
    MASK2( "FXAM",   gobble,     0xD9, 0xFF, 0xE5, X_NONE )
    MASK2( "FLD1",   gobble,     0xD9, 0xFF, 0xE8, X_NONE )
    MASK2( "FLDL2T", gobble,     0xD9, 0xFF, 0xE9, X_NONE )
    MASK2( "FLDL2E", gobble,     0xD9, 0xFF, 0xEA, X_NONE )
    MASK2( "FLDPI",  gobble,     0xD9, 0xFF, 0xEB, X_NONE )
    MASK2( "FLDLG2", gobble,     0xD9, 0xFF, 0xEC, X_NONE )
    MASK2( "FLDLN2", gobble,     0xD9, 0xFF, 0xED, X_NONE )
    MASK2( "FLDZ",   gobble,     0xD9, 0xFF, 0xEE, X_NONE )
    MASK2( "F2XM1",  gobble,     0xD9, 0xFF, 0xF0, X_NONE )
    MASK2( "FYL2X",  gobble,     0xD9, 0xFF, 0xF1, X_NONE )
    MASK2( "FPTAN",  gobble,     0xD9, 0xFF, 0xF2, X_NONE )
    MASK2( "FPATAN", gobble,     0xD9, 0xFF, 0xF3, X_NONE )
    MASK2( "FXTRACT",gobble,     0xD9, 0xFF, 0xF4, X_NONE )
    MASK2( "FDECSTP",gobble,     0xD9, 0xFF, 0xF6, X_NONE )
    MASK2( "FINCSTP",gobble,     0xD9, 0xFF, 0xF7, X_NONE )
    MASK2( "FPREM",  gobble,     0xD9, 0xFF, 0xF8, X_NONE )
    MASK2( "FYL2XP1",gobble,     0xD9, 0xFF, 0xF9, X_NONE )
    MASK2( "FSQRT",  gobble,     0xD9, 0xFF, 0xFA, X_NONE )
    MASK2( "FRNDINT",gobble,     0xD9, 0xFF, 0xFC, X_NONE )
    MASK2( "FSCALE", gobble,     0xD9, 0xFF, 0xFD, X_NONE )

    FP_MEM( "FLD",    fp_m32real, 0xD9, 0x00 )
    FP_MEM( "FST",    fp_m32real, 0xD9, 0x10 )
    FP_MEM( "FSTP",   fp_m32real, 0xD9, 0x18 )
    FP_MEM( "FLDENV", fp_menv,    0xD9, 0x20 )
    FP_MEM( "FLDCW",  fp_m16,     0xD9, 0x28 )
    FP_MEM( "FNSTENV",fp_menv,    0xD9, 0x30 )
    FP_MEM( "FNSTCW", fp_m16,     0xD9, 0x38 )

    FP_MEM( "FIADD",  fp_m32int,  0xDA, 0x00 )
    FP_MEM( "FIMUL",  fp_m32int,  0xDA, 0x08 )
    FP_MEM( "FICOM",  fp_m32int,  0xDA, 0x10 )
    FP_MEM( "FICOMP", fp_m32int,  0xDA, 0x18 )
    FP_MEM( "FISUB",  fp_m32int,  0xDA, 0x20 )
    FP_MEM( "FISUBR", fp_m32int,  0xDA, 0x28 )
    FP_MEM( "FIDIV",  fp_m32int,  0xDA, 0x30 )
    FP_MEM( "FIDIVR", fp_m32int,  0xDA, 0x38 )

    MASK2( "FNENI",   gobble,     0xDB, 0xFF, 0xE0, X_NONE )
    MASK2( "FNDISI",  gobble,     0xDB, 0xFF, 0xE1, X_NONE )
    MASK2( "FNCLEX",  gobble,     0xDB, 0xFF, 0xE2, X_NONE )
    MASK2( "FNINIT",  gobble,     0xDB, 0xFF, 0xE3, X_NONE )
    MASK2( "FNSETPM", gobble,     0xDB, 0xFF, 0xE4, X_NONE )

    FP_MEM( "FILD",   fp_m32int,  0xDB, 0x00 )
    FP_MEM( "FIST",   fp_m32int,  0xDB, 0x10 )
    FP_MEM( "FISTP",  fp_m32int,  0xDB, 0x18 )
    FP_MEM( "FLD",    fp_m80real, 0xDB, 0x28 )
    FP_MEM( "FSTP",   fp_m80real, 0xDB, 0x38 )

    MASK2( "FADD",   fp_sti_st,   0xDC, 0xF8, 0xC0, X_NONE )
    MASK2( "FMUL",   fp_sti_st,   0xDC, 0xF8, 0xC8, X_NONE )
    MASK2( "FSUBR",  fp_sti_st,   0xDC, 0xF8, 0xE0, X_NONE )
    MASK2( "FSUB",   fp_sti_st,   0xDC, 0xF8, 0xE8, X_NONE )
    MASK2( "FDIVR",  fp_sti_st,   0xDC, 0xF8, 0xF0, X_NONE )
    MASK2( "FDIV",   fp_sti_st,   0xDC, 0xF8, 0xF8, X_NONE )

    FP_MEM( "FADD",  fp_m64real, 0xDC, 0x00 )
    FP_MEM( "FMUL",  fp_m64real, 0xDC, 0x08 )
    FP_MEM( "FCOM",  fp_m64real, 0xDC, 0x10 )
    FP_MEM( "FCOMP", fp_m64real, 0xDC, 0x18 )
    FP_MEM( "FSUB",  fp_m64real, 0xDC, 0x20 )
    FP_MEM( "FSUBR", fp_m64real, 0xDC, 0x28 )
    FP_MEM( "FDIV",  fp_m64real, 0xDC, 0x30 )
    FP_MEM( "FDIVR", fp_m64real, 0xDC, 0x38 )

    MASK2( "FFREE",  fp_sti,      0xDD, 0xF8, 0xC0, X_NONE )
    MASK2( "FST",    fp_sti,      0xDD, 0xF8, 0xD0, X_NONE )
    MASK2( "FSTP",   fp_sti,      0xDD, 0xF8, 0xD8, X_NONE )

    FP_MEM( "FLD",    fp_m64real, 0xDD, 0x00 )
    FP_MEM( "FST",    fp_m64real, 0xDD, 0x10 )
    FP_MEM( "FSTP",   fp_m64real, 0xDD, 0x18 )
    FP_MEM( "FRSTOR", fp_menv,    0xDD, 0x20 )
    FP_MEM( "FNSAVE", fp_menv,    0xDD, 0x30 )
    FP_MEM( "FNSTSW", fp_m16,     0xDD, 0x38 )

    MASK2( "FADDP",  fp_sti_st,   0xDE, 0xF8, 0xC0, X_NONE )
    MASK2( "FMULP",  fp_sti_st,   0xDE, 0xF8, 0xC8, X_NONE )
    MASK2( "FCOMPP", gobble,      0xDE, 0xFF, 0xD9, X_NONE )
    MASK2( "FSUBRP", fp_sti_st,   0xDE, 0xF8, 0xE0, X_NONE )
    MASK2( "FSUBP",  fp_sti_st,   0xDE, 0xF8, 0xE8, X_NONE )
    MASK2( "FDIVRP", fp_sti_st,   0xDE, 0xF8, 0xF0, X_NONE )
    MASK2( "FDIVP",  fp_sti_st,   0xDE, 0xF8, 0xF8, X_NONE )

    FP_MEM( "FIADD",  fp_m16int, 0xDE, 0x00 )
    FP_MEM( "FIMUL",  fp_m16int, 0xDE, 0x08 )
    FP_MEM( "FICOM",  fp_m16int, 0xDE, 0x10 )
    FP_MEM( "FICOMP", fp_m16int, 0xDE, 0x18 )
    FP_MEM( "FISUB",  fp_m16int, 0xDE, 0x20 )
    FP_MEM( "FISUBR", fp_m16int, 0xDE, 0x28 )
    FP_MEM( "FIDIV",  fp_m16int, 0xDE, 0x30 )
    FP_MEM( "FIDIVR", fp_m16int, 0xDE, 0x38 )

    MASK2( "FNSTSW", gobble_AX,   0xDF, 0xFF, 0xE0, X_NONE )

    FP_MEM( "FILD",  fp_m16int,  0xDF, 0x00 )
    FP_MEM( "FIST",  fp_m16int,  0xDF, 0x10 )
    FP_MEM( "FISTP", fp_m16int,  0xDF, 0x18 )
    FP_MEM( "FBLD",  fp_m80bcd,  0xDF, 0x20 )
    FP_MEM( "FILD",  fp_m64int,  0xDF, 0x28 )
    FP_MEM( "FBSTP", fp_m80bcd,  0xDF, 0x30 )
    FP_MEM( "FISTP", fp_m64int,  0xDF, 0x38 )

    MASK( "ESC",     modrm, 0xF8, 0xD8, X_NONE )
    
    INSN( "LOCK",   none,  0xF0, X_NONE )
    
/*----------------------------------------------------------------------------
  SEGMENT OVERRIDE PREFIX
  ----------------------------------------------------------------------------*/

    PREFIX( pfx_seg, 0x26 ) /* ES */
    PREFIX( pfx_seg, 0x2E ) /* CS */
    PREFIX( pfx_seg, 0x36 ) /* SS */
    PREFIX( pfx_seg, 0x3E ) /* DS */
  
/*----------------------------------------------------------------------------*/

   END
};

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
