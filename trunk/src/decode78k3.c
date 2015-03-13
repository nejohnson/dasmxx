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
 * Gloabally-visible decoder properties
 *****************************************************************************/

DASM_PROFILE(   "dasm78k3",
                "NEC 78K/III",
                5,
                9,
                0
)

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/

/* Define this to use descriptive register names */
#define USE_ALT_REG_NAMES

/* Common address offsets */
#define SADDR_OFFSET            ( 0xFE00 )
#define SFR_OFFSET              ( 0xFF00 )

/* Common output formats */
#define FORMAT_NUM_8BIT         "$%02X"
#define FORMAT_NUM_16BIT        "$%04X"
#define FORMAT_REG              "R%d"

/* Construct a 16-bit word out of low and high bytes */
#define MK_WORD(l,h)            ( ((l) & 0xFF) | (((h) & 0xFF) << 8) )

/*****************************************************************************
 * Private data.
 *****************************************************************************/

/* Tables of register names used in addressing modes */

static const char * MEM_MOD_RI[8] = {
    "[DE+]",
    "[HL+]",
    "[DE-]",
    "[HL-]",
    "[DE]",
    "[HL]",
    "[VP]",
    "[UP]"
};

static const char * MEM_MOD_BI[6] = {
    "[DE+A]",
    "[HL+A]",
    "[DE+B]",
    "[HL+B]",
    "[VP+DE]",
    "[VP+HL]"
};

static const char * MEM_MOD_BASE[5] = {
    "[DE+",
    "[SP+",
    "[HL+",
    "[UP+",
    "[VP+"
};

static const char * MEM_MOD_INDEX[4] = {
    "[DE]",
    "[A]",
    "[HL]",
    "[B]"
};

#if defined(USE_ALT_REG_NAMES)

/* Single Registers */
#define REG_R0        "X"
#define REG_R1        "A"
#define REG_R2        "C"
#define REG_R3        "B"
#define REG_R4        "R4"
#define REG_R5        "R5"
#define REG_R6        "R6"
#define REG_R7        "R7"
#define REG_R8        "VPL"
#define REG_R9        "VPH"
#define REG_R10        "UPL"
#define REG_R11        "UPH"
#define REG_R12        "E"
#define REG_R13        "D"
#define REG_R14        "L"
#define REG_R15        "H"

/* Register Pairs */
#define REG_RP0        "AX"
#define REG_RP1        "BC"
#define REG_RP2        "RP2"
#define REG_RP3        "RP3"
#define REG_RP4        "VP"
#define REG_RP5        "UP"
#define REG_RP6        "DE"
#define REG_RP7        "HL"


#else

/* Single Registers */
#define REG_R0        "R0"
#define REG_R1        "R1"
#define REG_R2        "R2"
#define REG_R3        "R3"
#define REG_R4        "R4"
#define REG_R5        "R5"
#define REG_R6        "R6"
#define REG_R7        "R7"
#define REG_R8        "R8"
#define REG_R9        "R9"
#define REG_R10        "R10"
#define REG_R11        "R11"
#define REG_R12        "R12"
#define REG_R13        "R13"
#define REG_R14        "R14"
#define REG_R15        "R15"

/* Register Pairs */
#define REG_RP0        "RP0"
#define REG_RP1        "RP1"
#define REG_RP2        "RP2"
#define REG_RP3        "RP3"
#define REG_RP4        "RP4"
#define REG_RP5        "RP5"
#define REG_RP6        "RP6"
#define REG_RP7        "RP7"

#endif

static const char * R[16] = {
    REG_R0,
    REG_R1,
    REG_R2,
    REG_R3,
    REG_R4,
    REG_R5,
    REG_R6,
    REG_R7,
    REG_R8,
    REG_R9,
    REG_R10,
    REG_R11,
    REG_R12,
    REG_R13,
    REG_R14,
    REG_R15,
}; 

static const char * RP[8] = {
    REG_RP0,
    REG_RP1,
    REG_RP2,
    REG_RP3,
    REG_RP4,
    REG_RP5,
    REG_RP6,
    REG_RP7
};

static const char * RP1[8] = { /* Note permutated ordering! */
    REG_RP0,
    REG_RP4,
    REG_RP1,
    REG_RP5,
    REG_RP2,
    REG_RP6,
    REG_RP3,
    REG_RP7
};

static const char * RP2[4] = {
    "VP", "UP", "DE", "HL"
};

static const char * R2[2] = {
    "C", "B"
};

/*****************************************************************************
 *        Private Functions
 *****************************************************************************/

/***********************************************************
 *
 * FUNCTION
 *      emit_saddr
 *
 * DESCRIPTION
 *      Emits saddr and saddrp addresses, which are a little
 *      bit tricky.
 *      If the offset is in the range 00-1F then it is really
 *      an SFR in the range FF00-FF1F.  This is done to support
 *      shorter addressing so the most often used SFRs can be
 *      accessed a little bit quicker.  Deah oh dear, what a
 *      nasty little hack!
 *
 * RETURNS
 *      none
 *
 ************************************************************/
static void emit_saddr( UBYTE offset )
{
   ADDR saddr = offset + ( offset >= 0x20 ? SADDR_OFFSET : SFR_OFFSET );
    
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, saddr ) );
    xref_addxref( saddr >= SFR_OFFSET ? X_REG : X_PTR, g_insn_addr, saddr );
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

/******************************************************************************/
/**                            Single Operands                               **/
/******************************************************************************/

/***********************************************************
 * Process ".bit" operand.
 ************************************************************/

OPERAND_FUNC(bit)
{
    UBYTE bit = opc & 0x07;
    
    operand( ".%d", bit );
}

/***********************************************************
 * Process "r" operand.
 *    r comes from opc byte.
 ************************************************************/
 
OPERAND_FUNC(r)
{
    UBYTE r = opc & 0x0F;
    
    operand( "%s", R[r] );
}

/***********************************************************
 * Process "r1" operand.
 *    r1 comes from opc byte.
 ************************************************************/

OPERAND_FUNC(r1)
{
    UBYTE r1 = opc & 0x07;
    
    operand( "%s", R[r1] );
}

/***********************************************************
 * Process "r2" operand.
 *    r2 comes from opc byte.
 ************************************************************/

OPERAND_FUNC(r2)
{
    UBYTE r2 = opc & 0x01;
    
    operand( "%s", R2[r2] );
}

/***********************************************************
 * Process "r" operand.
 *    r comes from opc byte.
 ************************************************************/

OPERAND_FUNC(rp)
{
    UBYTE rp = opc & 0x07;
    
    operand( "%s", RP[rp] );
}

/***********************************************************
 * Process "rp1" operand.
 *    rp1 comes from opc byte.
 ************************************************************/

OPERAND_FUNC(rp1)
{
    UBYTE rp1 = opc & 0x07;
    
    operand( "%s", RP1[rp1] );
}

/***********************************************************
 * Process "rp2" operands.
 *    rp2 comes from opc byte.
 ************************************************************/

OPERAND_FUNC(rp2)
{
    UBYTE rp2 = opc & 0x03;
    
    operand( "%s", RP2[rp2] );
}

/***********************************************************
 * Process "RBn" operands.
 *    n comes from opc byte.
 ************************************************************/

OPERAND_FUNC(RBn)
{
    UBYTE n = opc & 0x07;
    
    operand( "RB%d", n );
}

/***********************************************************
 * Process "RBn,ALT" operands.
 *    n comes from opc byte.
 ************************************************************/

OPERAND_FUNC(RBn_ALT)
{
    UBYTE n = opc & 0x07;
    
    operand( "RB%d", n );
    COMMA;
    operand( "ALT" );
}

/***********************************************************
 * Process "#byte" operands.
 *    byte comes from next byte.
 ************************************************************/

OPERAND_FUNC(byte)
{
   UBYTE byte = next( f, addr );
    
    operand( "#" FORMAT_NUM_8BIT, byte );
}

/***********************************************************
 * Process "saddr" operands.
 *    saddr comes from next byte.
 ************************************************************/

OPERAND_FUNC(saddr)
{
   UBYTE saddr_offset = next( f, addr );
    
    emit_saddr( saddr_offset );
}

/***********************************************************
 * Process "saddrp" operands.
 *    saddr comes from next byte.
 ************************************************************/

OPERAND_FUNC(saddrp)
{
   UBYTE saddrp_offset = next( f, addr );
    
    emit_saddr( saddrp_offset );
}

/***********************************************************
 * Process "sfr" operands.
 *    sfr comes from next byte.
 ************************************************************/

OPERAND_FUNC(sfr)
{
   UBYTE sfr_offset = next( f, addr );
    
    if ( sfr_offset == 0xFE )
        operand( "PSWL" );
    else if ( sfr_offset == 0xFF )
        operand( "PSWH" );
    else
    {
        operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, sfr_offset + SFR_OFFSET ) );
        xref_addxref( X_REG, g_insn_addr, sfr_offset + SFR_OFFSET );
    }
}

/***********************************************************
 * Process "sfrp" operands.
 *    sfr comes from next byte.
 ************************************************************/

OPERAND_FUNC(sfrp)
{
   UBYTE sfr_offset = next( f, addr );
    
    if ( sfr_offset == 0xFC )
        operand( "SP" );
    else if ( sfr_offset == 0xFE )
        operand( "PSWL" );
    else if ( sfr_offset == 0xFF )
        operand( "PSWH" );
    else
    {
        operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, sfr_offset + SFR_OFFSET ) );
        xref_addxref( X_REG, g_insn_addr, sfr_offset + SFR_OFFSET );
    }
}

/***********************************************************
 * Process "mem" operand, which is a restricted subset
 *    of the register-indirect mode.
 *    mem comes from opc byte.
 ************************************************************/
 
OPERAND_FUNC(mem)
{
    UBYTE mem = opc & 0x07;
    
    operand( "%s", MEM_MOD_RI[mem] );
}

/***********************************************************
 * Process larger "mem" operand.
 *    quite a tricky jobby.
 ************************************************************/
 
OPERAND_FUNC(memmod)
{
    UBYTE mod = opc & 0x1F;
    UBYTE mem, low_offset, high_offset;
    
    mem = next( f, addr );
    mem = ( mem >> 4 ) & 0x07;
    
    if ( mod == 0x16 ) /* Register Indirect Addressing */
        operand_mem( f, addr, mem, xtype );
    else if ( mod == 0x17 ) /* Base Index Addressing */
        operand( "%s", MEM_MOD_BI[mem] );
    else if ( mod == 0x06 ) /* Base Addressing */
    {
       low_offset  = next( f, addr );
        operand( "%s" FORMAT_NUM_8BIT "]", MEM_MOD_BASE[mem], low_offset );
    }
    else if ( mod == 0x0A ) /* Index Addressing */
    {
        UWORD base;
        
        low_offset  = next( f, addr );
        high_offset = next( f, addr );        
        base        = MK_WORD(low_offset, high_offset);
        
        if ( xref_findaddrlabel( base ) )
        {
            operand( "%s%s", 
                         xref_genwordaddr( NULL, FORMAT_NUM_16BIT, base ), 
                        MEM_MOD_INDEX[mem] );
        }
        else if ( xref_findaddrlabel( base - 1 ) )
        {
            operand( "%s+1%s", 
                         xref_genwordaddr( NULL, FORMAT_NUM_16BIT, base - 1 ), 
                        MEM_MOD_INDEX[mem] );
        }
        else
        {
            operand( "$" FORMAT_ADDR "%s", 
                         base, 
                        MEM_MOD_INDEX[mem] );
        }
        xref_addxref( X_TABLE, g_insn_addr, base );
    }
}

/***********************************************************
 * Process "[addr5]" operand.
 ************************************************************/

OPERAND_FUNC(ind_addr5)
{
    UBYTE addr5 = opc & 0x1f;
    ADDR  vector = 0x0040 + ( 2 * addr5 );
    
    operand( "[" FORMAT_NUM_16BIT "]", vector );
    xref_addxref( xtype, g_insn_addr, vector );
}

/***********************************************************
 * Process "addr11" operand.
 ************************************************************/

OPERAND_FUNC(addr11)
{
    UBYTE low_addr = next( f, addr );
    ADDR addr11 = MK_WORD( low_addr, opc & 0x07 );
    
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr11 ) );
    xref_addxref( xtype, g_insn_addr, addr11 );
}

/***********************************************************
 * Process "addr16" operand.
 ************************************************************/

OPERAND_FUNC(addr16)
{
    UBYTE low_addr  = next( f, addr );
    UBYTE high_addr = next( f, addr );
    UWORD addr16    = MK_WORD( low_addr, high_addr );

    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr16 ) );
    xref_addxref( xtype, g_insn_addr, addr16 );
}

/***********************************************************
 * Process "$addr16" operand.
 ************************************************************/
 
OPERAND_FUNC(addr16_rel)
{
    BYTE jdisp = (BYTE)next( f, addr );
    ADDR addr16 = *addr + jdisp;
    
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr16 ) );
    xref_addxref( xtype, g_insn_addr, addr16 );
}

/***********************************************************
 * Process "word" operands.
 *    word comes from next 2 bytes.
 ************************************************************/
 
OPERAND_FUNC(word)
{
    UBYTE low_byte  = next( f, addr );
    UBYTE high_byte = next( f, addr );
    UWORD word      = MK_WORD( low_byte, high_byte );
    
    operand( "#%s", xref_genwordaddr( NULL, FORMAT_NUM_16BIT, word ) );
    xref_addxref( xtype, g_insn_addr, word );
}

/***********************************************************
 * Process "A" operands.
 ************************************************************/
 
OPERAND_FUNC(A)
{
    operand( "A" );
}

/***********************************************************
 * Process "CY" operands.
 ************************************************************/
 
OPERAND_FUNC(CY)
{
    operand( "CY" );
}

/***********************************************************
 * Process "SP" operands.
 ************************************************************/
 
OPERAND_FUNC(SP)
{
    operand( "SP" );
}

/***********************************************************
 * Process "post" operands.
 * post comes from next byte.
 ************************************************************/
 
OPERAND_FUNC(post)
{
    UBYTE post = next( f, addr );
    int bit;
    int comma = 0;
    
    for ( bit = 0; bit < 8; bit++ )
    {
        if ( post & BIT(bit) )
        {
            if ( comma )
                operand( "," );
            operand( "%s", RP[bit] );
            comma = 1;
        }
    }
}

/***********************************************************
 * Process "PSW" operands.
 ************************************************************/
 
OPERAND_FUNC(PSW)
{
    operand( "PSW" );
}

/***********************************************************
 * Process "[DE+]" operands.
 ************************************************************/
 
OPERAND_FUNC(DE_inc)
{
    operand( "[DE+]" );
}

/***********************************************************
 * Process "[DE-]" operands.
 ************************************************************/
 
OPERAND_FUNC(DE_dec)
{
    operand( "[DE-]" );
}

/***********************************************************
 * Process "[HL+]" operands.
 ************************************************************/

OPERAND_FUNC(HL_inc)
{
    operand( "[HL+]" );
}

/***********************************************************
 * Process "HL_dec" operands.
 ************************************************************/
 
OPERAND_FUNC(HL_dec)
{
    operand( "[HL-]" );
}

/******************************************************************************/
/**                           Double Operands                                **/
/******************************************************************************/

/***********************************************************
 * Process "r1,#byte" operands.
 *    r1 comes from opc byte, byte comes from next byte.
 ************************************************************/
 
OPERAND_FUNC(r1_byte)
{
    operand_r1( f, addr, opc, xtype );
    COMMA;
    operand_byte( f, addr, opc, xtype );
}

/***********************************************************
 * Process "A,#byte" operands.
 *    byte comes from next byte.
 ************************************************************/
 
OPERAND_FUNC(A_byte)
{
    operand_A( f, addr, opc, xtype );
    COMMA;
    operand_byte( f, addr, opc, xtype );
}

/***********************************************************
 * Process "saddr,#byte" operands.
 *    saddr comes from next byte, byte comes from next byte.
 ************************************************************/
 
OPERAND_FUNC(saddr_byte)
{
    operand_saddr( f, addr, opc, xtype );
    COMMA;
    operand_byte( f, addr, opc, xtype );
}

/***********************************************************
 * Process "sfr,#byte" operands.
 *    sfr comes from next byte, byte comes from next byte.
 ************************************************************/

OPERAND_FUNC(sfr_byte)
{
    operand_sfr( f, addr, opc, xtype );
    COMMA;
    operand_byte( f, addr, opc, xtype );
}

/***********************************************************
 * Process "r,r1" operands.
 *    both registers come from next byte.
 ************************************************************/
 
OPERAND_FUNC(r_r1)
{
    UBYTE regs = next( f, addr );
    
    operand_r( f, addr, regs >> 4, xtype );
    COMMA;
    operand_r1( f, addr, regs, xtype );
}

/***********************************************************
 * Process "rp,rp1" operands.
 *    both registers come from next byte.
 ************************************************************/
 
OPERAND_FUNC(rp_rp1)
{
    UBYTE regs = next( f, addr );
    
    operand_rp( f, addr, regs >> 5, xtype );
    COMMA;
    operand_rp1( f, addr, regs, xtype );
}

/***********************************************************
 * Process "A,r1" operands.
 *    r1 come from opc byte.
 ************************************************************/
 
OPERAND_FUNC(A_r1)
{
    operand_A( f, addr, opc, xtype );
    COMMA;
    operand_r1( f, addr, opc, xtype );
}

/***********************************************************
 * Process "A,saddr" operands.
 ************************************************************/
 
OPERAND_FUNC(A_saddr)
{
    operand_A( f, addr, opc, xtype );
    COMMA;
    operand_saddr( f, addr, opc, xtype );
}

/***********************************************************
 * Process "saddr,A" operands.
 ************************************************************/
 
OPERAND_FUNC(saddr_A)
{
    operand_saddr( f, addr, opc, xtype );
    COMMA;
    operand_A( f, addr, opc, xtype );
}

/***********************************************************
 * Process "saddr,saddr" operands.
 *
 * Nasty: the instruction encodes the source offset followed
 * by the destination offset, so we need to extract the 
 * offsets and present them in the opposite order.
 ************************************************************/

OPERAND_FUNC(saddr_saddr)
{
    UBYTE saddr_src_offset = next( f, addr );
    UBYTE saddr_dst_offset = next( f, addr );
    
    emit_saddr( saddr_dst_offset );
    COMMA;
    emit_saddr( saddr_src_offset );
}

/***********************************************************
 * Process "A,sfr" operands.
 ************************************************************/

OPERAND_FUNC(A_sfr)
{
    operand_A( f, addr, opc, xtype );
    COMMA;
    operand_sfr( f, addr, opc, xtype );
}

/***********************************************************
 * Process "sfr,A" operands.
 ************************************************************/

OPERAND_FUNC(sfr_A)
{
    operand_sfr( f, addr, opc, xtype );
    COMMA;
    operand_A( f, addr, opc, xtype );
}

/***********************************************************
 * Process "A,mem" operands.
 *    mem comes from opc byte translated.
 ************************************************************/

OPERAND_FUNC(A_mem)
{
    operand_A( f, addr, opc, xtype );
    COMMA;
    operand_mem( f, addr, opc, xtype );
}

/***********************************************************
 * Process "mem,A" operands.
 *    mem comes from opc byte translated.
 ************************************************************/
 
OPERAND_FUNC(mem_A)
{
    operand_mem( f, addr, opc, xtype );
    COMMA;
    operand_A( f, addr, opc, xtype );
}

/***********************************************************
 * Process larger "A,mem" operands.
 ************************************************************/
 
OPERAND_FUNC(A_memmod)
{
    operand_A( f, addr, opc, xtype );
    COMMA;
    operand_memmod( f, addr, opc, xtype );
}

/***********************************************************
 * Process larger "mem,A" operands.
 ************************************************************/
 
OPERAND_FUNC(memmod_A)
{
    operand_memmod( f, addr, opc, xtype );
    COMMA;
    operand_A( f, addr, opc, xtype );
}

/***********************************************************
 * Process "A,[saddrp]" operands.
 ************************************************************/
 
OPERAND_FUNC(A_saddrp)
{
    operand_A( f, addr, opc, xtype );
    COMMA;
    operand( "[" );
    operand_saddrp( f, addr, opc, xtype );
    operand( "]" );
}

/***********************************************************
 * Process "[saddrp],A" operands.
 ************************************************************/
 
OPERAND_FUNC(saddrp_A)
{
    operand( "[" );
    operand_saddrp( f, addr, opc, xtype );
    operand( "]" );
    COMMA;
    operand_A( f, addr, opc, xtype );
}

/***********************************************************
 * Process "A,!addr16" operands.
 ************************************************************/

OPERAND_FUNC(A_addr16)
{
    operand_A( f, addr, opc, xtype );
    COMMA;
    operand_addr16( f, addr, opc, xtype );
}
 
/***********************************************************
 * Process "!addr16,A" operands.
 ************************************************************/

OPERAND_FUNC(addr16_A)
{
    operand_addr16( f, addr, opc, xtype );
    COMMA;
    operand_A( f, addr, opc, xtype );
}

/***********************************************************
 * Process "rp1,#word" operands.
 ************************************************************/
 
OPERAND_FUNC(rp1_word)
{
    operand_rp1( f, addr, opc, xtype );
    COMMA;
    operand_word( f, addr, opc, xtype );
}

/***********************************************************
 * Process "saddrp,#word" operands.
 ************************************************************/
 
OPERAND_FUNC(saddrp_word)
{
    operand_saddrp( f, addr, opc, xtype );
    COMMA;
    operand_word( f, addr, opc, xtype );
}

/***********************************************************
 * Process "sfrp,#word" operands.
 ************************************************************/
 
OPERAND_FUNC(sfrp_word)
{
    operand_sfrp( f, addr, opc, xtype );
    COMMA;
    operand_word( f, addr, opc, xtype );
}

/***********************************************************
 * Process "AX,saddrp" operands.
 ************************************************************/
 
OPERAND_FUNC(AX_saddrp)
{
    operand( "AX" );
    COMMA;
    operand_saddrp( f, addr, opc, xtype );
}

/***********************************************************
 * Process "saddr,A" operands.
 ************************************************************/
 
OPERAND_FUNC(saddrp_AX)
{
    operand_saddrp( f, addr, opc, xtype );
    COMMA;
    operand( "AX" );
}

/***********************************************************
 * Process "saddrp,saddrp" operands.
 *
 * Nasty: the instruction encodes the source offset followed
 * by the destination offset, so we need to extract the 
 * offsets and present them in the opposite order.
 ************************************************************/
 
OPERAND_FUNC(saddrp_saddrp)
{
    UBYTE saddr_src_offset = next( f, addr );
    UBYTE saddr_dst_offset = next( f, addr );
    
    emit_saddr( saddr_dst_offset );
    COMMA;
    emit_saddr( saddr_src_offset );
}

/***********************************************************
 * Process "AX,sfrp" operands.
 ************************************************************/
 
OPERAND_FUNC(AX_sfrp)
{
    operand( "AX" );
    COMMA;
    operand_sfrp( f, addr, opc, xtype );
}

/***********************************************************
 * Process "sfrp,AX" operands.
 ************************************************************/
 
OPERAND_FUNC(sfrp_AX)
{
    operand_sfrp( f, addr, opc, xtype );
    COMMA;
    operand( "AX" );
}

/***********************************************************
 * Process "rp1,!addr16" operands.
 ************************************************************/

OPERAND_FUNC(rp1_addr16)
{
    operand_rp1( f, addr, opc, xtype );
    COMMA;
    operand_addr16( f, addr, opc, xtype );
}

/***********************************************************
 * Process "!addr16,rp1" operands.
 ************************************************************/

OPERAND_FUNC(addr16_rp1)
{
    operand_addr16( f, addr, opc, xtype );
    COMMA;
    operand_rp1( f, addr, opc, xtype );
}

/***********************************************************
 * Process "AX,#word" operands.
 ************************************************************/
 
OPERAND_FUNC(AX_word)
{
    operand( "AX" );
    COMMA;
    operand_word( f, addr, opc, xtype );
}

/***********************************************************
 * Process "r1,n" operands.
 * n comes from next byte.
 ************************************************************/
 
OPERAND_FUNC(r1_n)
{
   UBYTE args = next( f, addr );
    
    operand_r1( f, addr, args, xtype );
    COMMA;
    operand( "%d", ( args >> 3 ) & 0x07 );
}

/***********************************************************
 * Process "rp1,n" operands.
 * n comes from next byte.
 ************************************************************/
 
OPERAND_FUNC(rp1_n)
{
   UBYTE args = next( f, addr );
    
    operand_rp1( f, addr, args, xtype );
    COMMA;
    operand( "%d", ( args >> 3 ) & 0x07 );
}

/***********************************************************
 * Process "[rp1]" operands.
 ************************************************************/
 
OPERAND_FUNC(rp1_ind)
{
    operand( "[" );
    operand_rp1( f, addr, opc, xtype );
    operand( "]" );
}

/***********************************************************
 * Process "saddr.bit" operands.
 ************************************************************/
 
OPERAND_FUNC(saddr_bit)
{
    operand_saddr( f, addr, opc, xtype );
    operand_bit( f, addr, opc, xtype );
}

/***********************************************************
 * Process "sfr.bit" operands.
 ************************************************************/
 
OPERAND_FUNC(sfr_bit)
{
    operand_sfr( f, addr, opc, xtype );
    operand_bit( f, addr, opc, xtype );
}

/***********************************************************
 * Process "A.bit" operands.
 ************************************************************/
 
OPERAND_FUNC(A_bit)
{
    operand_A( f, addr, opc, xtype );
    operand_bit( f, addr, opc, xtype );
}

/***********************************************************
 * Process "X.bit" operands.
 ************************************************************/
 
OPERAND_FUNC(X_bit)
{
    operand( "X" );
    operand_bit( f, addr, opc, xtype );
}

/***********************************************************
 * Process "PSWL.bit" operands.
 ************************************************************/
 
OPERAND_FUNC(PSWL_bit)
{
    operand( "PSWL" );
    operand_bit( f, addr, opc, xtype );
}

/***********************************************************
 * Process "PSWH.bit" operands.
 ************************************************************/
 
OPERAND_FUNC(PSWH_bit)
{
    operand( "PSWH" );
    operand_bit( f, addr, opc, xtype );
}

/***********************************************************
 * Process "CY,saddr.bit" operands.
 ************************************************************/
 
OPERAND_FUNC(CY_saddr_bit)
{
    operand_CY( f, addr, opc, xtype );
    COMMA;
    operand_saddr_bit( f, addr, opc, xtype );
}

/***********************************************************
 * Process "CY,sfr.bit" operands.
 ************************************************************/
 
OPERAND_FUNC(CY_sfr_bit)
{
    operand_CY( f, addr, opc, xtype );
    COMMA;
    operand_sfr_bit( f, addr, opc, xtype );
}

/***********************************************************
 * Process "CY,A.bit" operands.
 ************************************************************/
 
OPERAND_FUNC(CY_A_bit)
{
    operand_CY( f, addr, opc, xtype );
    COMMA;
    operand_A_bit( f, addr, opc, xtype );
}

/***********************************************************
 * Process "CY,X.bit" operands.
 ************************************************************/
 
OPERAND_FUNC(CY_X_bit)
{
    operand_CY( f, addr, opc, xtype );
    COMMA;
    operand_X_bit( f, addr, opc, xtype );
}

/***********************************************************
 * Process "CY,PSWL.bit" operands.
 ************************************************************/
 
OPERAND_FUNC(CY_PSWL_bit)
{
    operand_CY( f, addr, opc, xtype );
    COMMA;
    operand_PSWL_bit( f, addr, opc, xtype );
}

/***********************************************************
 * Process "CY,PSWH.bit" operands.
 ************************************************************/
 
OPERAND_FUNC(CY_PSWH_bit)
{
    operand_CY( f, addr, opc, xtype );
    COMMA;
    operand_PSWH_bit( f, addr, opc, xtype );
}

/***********************************************************
 * Process "CY,/saddr.bit" operands.
 ************************************************************/
 
OPERAND_FUNC(CY_n_saddr_bit)
{
    operand_CY( f, addr, opc, xtype );
    COMMA;
    operand( "/" );
    operand_saddr_bit( f, addr, opc, xtype );
}

/***********************************************************
 * Process "CY,/sfr.bit" operands.
 ************************************************************/
 
OPERAND_FUNC(CY_n_sfr_bit)
{
    operand_CY( f, addr, opc, xtype );
    COMMA;
    operand( "/" );
    operand_sfr_bit( f, addr, opc, xtype );
}

/***********************************************************
 * Process "CY,/A.bit" operands.
 ************************************************************/
 
OPERAND_FUNC(CY_n_A_bit)
{
    operand_CY( f, addr, opc, xtype );
    COMMA;
    operand( "/" );
    operand_A_bit( f, addr, opc, xtype );
}

/***********************************************************
 * Process "CY,/X.bit" operands.
 ************************************************************/
 
OPERAND_FUNC(CY_n_X_bit)
{
    operand_CY( f, addr, opc, xtype );
    COMMA;
    operand( "/" );
    operand_X_bit( f, addr, opc, xtype );
}

/***********************************************************
 * Process "CY,/PSWL.bit" operands.
 ************************************************************/
 
OPERAND_FUNC(CY_n_PSWL_bit)
{
    operand_CY( f, addr, opc, xtype );
    COMMA;
    operand( "/" );
    operand_PSWL_bit( f, addr, opc, xtype );
}

/***********************************************************
 * Process "CY,/PSWH.bit" operands.
 ************************************************************/
 
OPERAND_FUNC(CY_n_PSWH_bit)
{
    operand_CY( f, addr, opc, xtype );
    COMMA;
    operand( "/" );
    operand_PSWH_bit( f, addr, opc, xtype );
}

/***********************************************************
 * Process "saddr.bit,CY" operands.
 ************************************************************/
 
OPERAND_FUNC(saddr_bit_CY)
{
    operand_saddr_bit( f, addr, opc, xtype );
    COMMA;
    operand_CY( f, addr, opc, xtype );
}

/***********************************************************
 * Process "sfr.bit,CY" operands.
 ************************************************************/
 
OPERAND_FUNC(sfr_bit_CY)
{
    operand_sfr_bit( f, addr, opc, xtype );
    COMMA;
    operand_CY( f, addr, opc, xtype );
}

/***********************************************************
 * Process "A.bit,CY" operands.
 ************************************************************/
 
OPERAND_FUNC(A_bit_CY)
{
    operand_A_bit( f, addr, opc, xtype );
    COMMA;
    operand_CY( f, addr, opc, xtype );
}

/***********************************************************
 * Process "X.bit,CY" operands.
 ************************************************************/
 
OPERAND_FUNC(X_bit_CY)
{
    operand_X_bit( f, addr, opc, xtype );
    COMMA;
    operand_CY( f, addr, opc, xtype );
}

/***********************************************************
 * Process "PSWL.bit,CY" operands.
 ************************************************************/
 
OPERAND_FUNC(PSWL_bit_CY)
{
    operand_PSWL_bit( f, addr, opc, xtype );
    COMMA;
    operand_CY( f, addr, opc, xtype );
}

/***********************************************************
 * Process "PSWH.bit,CY" operands.
 ************************************************************/
 
OPERAND_FUNC(PSWH_bit_CY)
{
    operand_PSWH_bit( f, addr, opc, xtype );
    COMMA;
    operand_CY( f, addr, opc, xtype );
}

/***********************************************************
 * Process "saddr.bit,$addr16" operands.
 ************************************************************/

OPERAND_FUNC(saddr_bit_addr16_rel)
{
    operand_saddr_bit( f, addr, opc, xtype );
    COMMA;
    operand_addr16_rel( f, addr, opc, xtype );
}

/***********************************************************
 * Process "sfr.bit,$addr16" operands.
 ************************************************************/

OPERAND_FUNC(sfr_bit_addr16_rel)
{
    operand_sfr_bit( f, addr, opc, xtype );
    COMMA;
    operand_addr16_rel( f, addr, opc, xtype );
}

/***********************************************************
 * Process "A.bit,$addr16" operands.
 ************************************************************/

OPERAND_FUNC(A_bit_addr16_rel)
{
    operand_A_bit( f, addr, opc, xtype );
    COMMA;
    operand_addr16_rel( f, addr, opc, xtype );
}

/***********************************************************
 * Process "X.bit,$addr16" operands.
 ************************************************************/

OPERAND_FUNC(X_bit_addr16_rel)
{
    operand_X_bit( f, addr, opc, xtype );
    COMMA;
    operand_addr16_rel( f, addr, opc, xtype );
}

/***********************************************************
 * Process "PSWH.bit,$addr16" operands.
 ************************************************************/

OPERAND_FUNC(PSWH_bit_addr16_rel)
{
    operand_PSWH_bit( f, addr, opc, xtype );
    COMMA;
    operand_addr16_rel( f, addr, opc, xtype );
}

/***********************************************************
 * Process "PSWL.bit,$addr16" operands.
 ************************************************************/

OPERAND_FUNC(PSWL_bit_addr16_rel)
{
    operand_PSWL_bit( f, addr, opc, xtype );
    COMMA;
    operand_addr16_rel( f, addr, opc, xtype );
}

/***********************************************************
 * Process "r2,$addr16" operands.
 ************************************************************/

OPERAND_FUNC(r2_addr16_rel)
{
    operand_r2( f, addr, opc, xtype );
    COMMA;
    operand_addr16_rel( f, addr, opc, xtype );
}

/***********************************************************
 * Process "saddr,$addr16" operands.
 ************************************************************/

OPERAND_FUNC(saddr_addr16_rel)
{
    operand_saddr( f, addr, opc, xtype );
    COMMA;
    operand_addr16_rel( f, addr, opc, xtype );
}

/***********************************************************
 * Process "[DE+],A" operands.
 ************************************************************/

OPERAND_FUNC(DE_inc_A)
{
    operand_DE_inc( f, addr, opc, xtype );
    COMMA;
    operand_A( f, addr, opc, xtype );
}

/***********************************************************
 * Process "[DE-],A" operands.
 ************************************************************/

OPERAND_FUNC(DE_dec_A)
{
    operand_DE_dec( f, addr, opc, xtype );
    COMMA;
    operand_A( f, addr, opc, xtype );
}

/***********************************************************
 * Process "[DE+],[HL+]" operands.
 ************************************************************/

OPERAND_FUNC(DE_inc_HL_inc)
{
    operand_DE_inc( f, addr, opc, xtype );
    COMMA;
    operand_HL_inc( f, addr, opc, xtype );
}

/***********************************************************
 * Process "[DE-],[HL-]" operands.
 ************************************************************/

OPERAND_FUNC(DE_dec_HL_dec)
{
    operand_DE_dec( f, addr, opc, xtype );
    COMMA;
    operand_HL_dec( f, addr, opc, xtype );
}

/***********************************************************
 * Process "STBC,#byte" operands.
 * This is a weird one, as the byte is actually stored first as the 
 * inverted value followed by the normal value.
 * In thise case we simply consume the inverted byte and 
 * present the normal byte.
 ************************************************************/

OPERAND_FUNC(STBC_byte)
{
    (void)next( f, addr );
    
    operand( "STBC" );
    COMMA;
    operand_byte( f, addr, opc, xtype );
}

/***********************************************************
 * Process "WDM,#byte" operands.
 * This is a weird one, as the byte is actually stored first as the 
 * inverted value followed by the normal value.
 * In thise case we simply consume the inverted byte and 
 * present the normal byte.
 ************************************************************/

OPERAND_FUNC(WDM_byte)
{
    (void)next( f, addr );
    
    operand( "WDM" );
    COMMA;
    operand_byte( f, addr, opc, xtype );
}

/******************************************************************************/
/** Instruction Decoding Tables                                              **/
/** Note: tables are here as they refer to operand functions defined above.  **/
/******************************************************************************/

static optab_t optab_01[] = {
    
    INSN  ( "xch",  A_sfr,     0x21, X_NONE )
    INSN  ( "xchw", AX_sfrp,   0x1B, X_NONE ) 
    
#undef BYTE_OP
#define BYTE_OP(M_name, M_code)    INSN  ( M_name,  sfr_byte,    ( 0x60 | M_code ), X_NONE ) \
                                            INSN  ( M_name,  A_sfr,       ( 0x90 | M_code ), X_NONE )
                                        
    BYTE_OP( "add",  0x08 )
    BYTE_OP( "addc", 0x09 )
    BYTE_OP( "sub",  0x0A )
    BYTE_OP( "subc", 0x0B )
    BYTE_OP( "and",  0x0C )
    BYTE_OP( "or",   0x0E )
    BYTE_OP( "xor",  0x0D )
    BYTE_OP( "cmp",  0x0F )

#undef WORD_OP
#define WORD_OP(M_name, M_code)    INSN  ( M_name,  sfrp_word,    ( 0x00 | M_code ), X_NONE ) \
                                            INSN  ( M_name,  AX_sfrp,      ( 0x10 | M_code ), X_NONE )
                                            
    WORD_OP( "addw", 0x0D )
    WORD_OP( "subw", 0x0E )
    WORD_OP( "cmpw", 0x0F )
    
    END
};

/******************************************************************************/

static optab_t optab_02[] = {
    
    RANGE ( "mov1",  CY_PSWH_bit,  0x08, 0x0F, X_NONE )
    RANGE ( "mov1",  CY_PSWL_bit,  0x00, 0x07, X_NONE )
    RANGE ( "mov1",  PSWH_bit_CY,  0x18, 0x1F, X_NONE )
    RANGE ( "mov1",  PSWL_bit_CY,  0x10, 0x17, X_NONE )
    
    RANGE ( "and1",  CY_PSWH_bit,    0x28, 0x2F, X_NONE )
    RANGE ( "and1",  CY_n_PSWH_bit,  0x38, 0x3F, X_NONE )
    RANGE ( "and1",  CY_PSWL_bit,    0x20, 0x27, X_NONE )
    RANGE ( "and1",  CY_n_PSWL_bit,  0x30, 0x37, X_NONE )

    RANGE ( "or1",   CY_PSWH_bit,    0x48, 0x4F, X_NONE )
    RANGE ( "or1",   CY_n_PSWH_bit,  0x58, 0x5F, X_NONE )
    RANGE ( "or1",   CY_PSWL_bit,    0x40, 0x47, X_NONE )
    RANGE ( "or1",   CY_n_PSWL_bit,  0x50, 0x57, X_NONE )
    
    RANGE ( "xor1",  CY_PSWH_bit,    0x68, 0x6F, X_NONE )
    RANGE ( "xor1",  CY_PSWL_bit,    0x60, 0x67, X_NONE )

    RANGE ( "set1",  PSWH_bit,       0x88, 0x8F, X_NONE )
    RANGE ( "set1",  PSWL_bit,       0x80, 0x87, X_NONE )
    
    RANGE ( "clr1",  PSWH_bit,       0x98, 0x9F, X_NONE )
    RANGE ( "clr1",  PSWL_bit,       0x90, 0x97, X_NONE )
    
    RANGE ( "not1",  PSWH_bit,       0x78, 0x7F, X_NONE )
    RANGE ( "not1",  PSWL_bit,       0x70, 0x77, X_NONE )
    
    RANGE ( "bt",    PSWH_bit_addr16_rel, 0xB8, 0xBF, X_JMP )
    RANGE ( "bt",    PSWL_bit_addr16_rel, 0xB0, 0xB7, X_JMP )
    
    RANGE ( "bf",    PSWH_bit_addr16_rel, 0xA8, 0xAF, X_JMP )
    RANGE ( "bf",    PSWL_bit_addr16_rel, 0xA0, 0xA7, X_JMP )
    
    RANGE ( "btclr", PSWH_bit_addr16_rel, 0xD8, 0xDF, X_JMP )
    RANGE ( "btclr", PSWL_bit_addr16_rel, 0xD0, 0xD7, X_JMP )
    
    RANGE ( "bfset", PSWH_bit_addr16_rel, 0xC8, 0xCF, X_JMP )
    RANGE ( "bfset", PSWL_bit_addr16_rel, 0xC0, 0xC7, X_JMP )
    
    END
};

/******************************************************************************/

static optab_t optab_03[] = {
    
    INSN  ( "incw",  saddrp,     0xE8, X_NONE )
    INSN  ( "decw",  saddrp,     0xE9, X_NONE )
    
    RANGE ( "mov1",  CY_A_bit,   0x08, 0x0F, X_NONE )
    RANGE ( "mov1",  CY_X_bit,   0x00, 0x07, X_NONE )
    RANGE ( "mov1",  A_bit_CY,   0x18, 0x1F, X_NONE )
    RANGE ( "mov1",  X_bit_CY,   0x10, 0x17, X_NONE )
    
    RANGE ( "and1",  CY_A_bit,   0x28, 0x2F, X_NONE )
    RANGE ( "and1",  CY_n_A_bit, 0x38, 0x3F, X_NONE )
    RANGE ( "and1",  CY_X_bit,   0x20, 0x27, X_NONE )
    RANGE ( "and1",  CY_n_X_bit, 0x30, 0x37, X_NONE )
    
    RANGE ( "or1",   CY_A_bit,   0x48, 0x4F , X_NONE)
    RANGE ( "or1",   CY_n_A_bit, 0x58, 0x5F, X_NONE )
    RANGE ( "or1",   CY_X_bit,   0x40, 0x47, X_NONE )
    RANGE ( "or1",   CY_n_X_bit, 0x50, 0x57, X_NONE )
    
    RANGE ( "xor1",  CY_A_bit,   0x68, 0x6F, X_NONE )
    RANGE ( "xor1",  CY_X_bit,   0x60, 0x67, X_NONE )
    
    RANGE ( "set1",  A_bit,      0x88, 0x8F, X_NONE )
    RANGE ( "set1",  X_bit,      0x80, 0x87, X_NONE )
    
    RANGE ( "clr1",  A_bit,      0x98, 0x9F, X_NONE )
    RANGE ( "clr1",  X_bit,      0x90, 0x97, X_NONE )
    
    RANGE ( "not1",  A_bit,      0x78, 0x7F, X_NONE )
    RANGE ( "not1",  X_bit,      0x70, 0x77, X_NONE )
    
    RANGE ( "bt",    A_bit_addr16_rel, 0xB8, 0xBF, X_JMP )
    RANGE ( "bt",    X_bit_addr16_rel, 0xB0, 0xB7, X_JMP )
    
    RANGE ( "bf",    A_bit_addr16_rel, 0xA8, 0xAF, X_JMP )
    RANGE ( "bf",    X_bit_addr16_rel, 0xA0, 0xA7, X_JMP )
    
    RANGE ( "btclr", A_bit_addr16_rel, 0xD8, 0xDF, X_JMP )
    RANGE ( "btclr", X_bit_addr16_rel, 0xD0, 0xD7, X_JMP )
    
    RANGE ( "bfset", A_bit_addr16_rel, 0xC8, 0xCF, X_JMP )
    RANGE ( "bfset", X_bit_addr16_rel, 0xC0, 0xC7, X_JMP )
    
    END
};

/******************************************************************************/

static optab_t optab_05[] = {
    
    RANGE ( "mulu",  r1,       0x08, 0x0F, X_NONE )
    RANGE ( "divuw", r1,       0x18, 0x1F, X_NONE )
    RANGE ( "muluw", rp1,      0x28, 0x2F, X_NONE )
    RANGE ( "divux", rp1,      0xE8, 0xEF, X_NONE )
    
    RANGE ( "ror4",  rp1_ind,  0x88, 0x8F, X_NONE )
    RANGE ( "rol4",  rp1_ind,  0x98, 0x9F, X_NONE )
    
    RANGE ( "call",  rp1,      0x58, 0x5F, X_CALL )
    RANGE ( "call",  rp1_ind,  0x78, 0x7F, X_CALL )
    
    INSN  ( "incw",  SP,       0xC8, X_NONE )
    INSN  ( "decw",  SP,       0xC9, X_NONE )
    
    RANGE ( "br",    rp1,      0x48, 0x4F, X_JMP )
    RANGE ( "br",    rp1_ind,  0x68, 0x6F, X_JMP )
    
    RANGE ( "brkcs", RBn,      0xD8, 0xDF, X_NONE )
    
    RANGE ( "sel",   RBn,      0xA8, 0xAF, X_NONE )
    RANGE ( "sel",   RBn_ALT,  0xB8, 0xBF, X_NONE )
    
    END
};

/******************************************************************************/

static optab_t optab_07[] = {
    
    INSN  ( "incw",  saddrp,     0xE8, X_NONE )
    INSN  ( "decw",  saddrp,     0xE9, X_NONE )
    
    INSN  ( "bgt",   addr16_rel, 0xFB, X_JMP )
    INSN  ( "bge",   addr16_rel, 0xF9, X_JMP )
    INSN  ( "blt",   addr16_rel, 0xF8, X_JMP )
    INSN  ( "ble",   addr16_rel, 0xFA, X_JMP )
    INSN  ( "bh",    addr16_rel, 0xFD, X_JMP )
    INSN  ( "bnh",   addr16_rel, 0xFC, X_JMP )
    
    END
};

/******************************************************************************/

static optab_t optab_08[] = {
    
    RANGE ( "mov1", CY_saddr_bit,        0x00, 0x07, X_NONE )
    RANGE ( "mov1", CY_sfr_bit,       0x08, 0x0F, X_NONE )
    RANGE ( "mov1", saddr_bit_CY,     0x10, 0x17, X_NONE )
    RANGE ( "mov1", sfr_bit_CY,       0x18, 0x1F, X_NONE )
    
    RANGE ( "and1", CY_saddr_bit,        0x20, 0x27, X_NONE )
    RANGE ( "and1", CY_n_saddr_bit,    0x30, 0x37, X_NONE )
    RANGE ( "and1", CY_sfr_bit,       0x28, 0x2F, X_NONE )
    RANGE ( "and1", CY_n_sfr_bit,     0x38, 0x3F, X_NONE )
    
    RANGE ( "or1",  CY_saddr_bit,        0x40, 0x47, X_NONE )
    RANGE ( "or1",  CY_n_saddr_bit,    0x50, 0x57, X_NONE )
    RANGE ( "or1",  CY_sfr_bit,       0x48, 0x4F, X_NONE )
    RANGE ( "or1",  CY_n_sfr_bit,     0x58, 0x5F, X_NONE )
    
    RANGE ( "xor1", CY_saddr_bit,        0x60, 0x67, X_NONE )
    RANGE ( "xor1", CY_sfr_bit,       0x68, 0x6F, X_NONE )
    
    RANGE ( "set1", sfr_bit,          0x88, 0x8F, X_NONE )
    RANGE ( "clr1", sfr_bit,          0x98, 0x9F, X_NONE )
    
    RANGE ( "not1", saddr_bit,        0x70, 0x77, X_NONE )
    RANGE ( "not1", sfr_bit,          0x78, 0x7F, X_NONE )
    
    RANGE ( "bt",   sfr_bit_addr16_rel,    0xB8, 0xBF, X_JMP )
    
    RANGE ( "bf",   saddr_bit_addr16_rel,  0xA0, 0xA7, X_JMP )
    RANGE ( "bf",   sfr_bit_addr16_rel,    0xA8, 0xAF, X_JMP )
    
    RANGE ( "btclr", saddr_bit_addr16_rel, 0xD0, 0xD7, X_JMP )
    RANGE ( "btclr", sfr_bit_addr16_rel,   0xD8, 0xDF, X_JMP )
    
    RANGE ( "bfset", saddr_bit_addr16_rel, 0xC0, 0xC7, X_JMP )
    RANGE ( "bfset", sfr_bit_addr16_rel,   0xC8, 0xCF, X_JMP )
    
    END
};

/******************************************************************************/

static optab_t optab_09[] = {
    
    INSN  ( "mov",  A_addr16,   0xF0, X_DATA )
    INSN  ( "mov",  addr16_A,   0xF1, X_DATA )
    
    RANGE ( "movw", rp1_addr16, 0x80, 0x87, X_DATA )
    RANGE ( "movw", addr16_rp1, 0x90, 0x97, X_DATA )

    INSN  ( "mov",  STBC_byte,  0x44, X_NONE )
    INSN  ( "mov",  WDM_byte,   0x42, X_NONE )

    END
};

/******************************************************************************/

static optab_t optab_15[] = {
    
    INSN  ( "movm",    DE_inc_A,       0x00, X_NONE )
    INSN  ( "movm",    DE_dec_A,       0x10, X_NONE )
    INSN  ( "movbk",   DE_inc_HL_inc,  0x20, X_NONE )
    INSN  ( "movbk",   DE_dec_HL_dec,  0x30, X_NONE )
    
    INSN  ( "xchm",    DE_inc_A,       0x01, X_NONE )
    INSN  ( "xchm",    DE_dec_A,       0x11, X_NONE )
    INSN  ( "xchbk",   DE_inc_HL_inc,  0x21, X_NONE )
    INSN  ( "xchbk",   DE_dec_HL_dec,  0x31, X_NONE )
    
    INSN  ( "cmpme",   DE_inc_A,       0x04, X_NONE )
    INSN  ( "cmpme",   DE_dec_A,       0x14, X_NONE )
    INSN  ( "cmpbke",  DE_inc_HL_inc,  0x24, X_NONE )
    INSN  ( "cmpbke",  DE_dec_HL_dec,  0x34, X_NONE )
    
    INSN  ( "cmpmne",  DE_inc_A,       0x05, X_NONE )
    INSN  ( "cmpmne",  DE_dec_A,       0x15, X_NONE )
    INSN  ( "cmpbkne", DE_inc_HL_inc,  0x25, X_NONE )
    INSN  ( "cmpbkne", DE_dec_HL_dec,  0x35, X_NONE )
    
    INSN  ( "cmpmc",   DE_inc_A,       0x07, X_NONE )
    INSN  ( "cmpmc",   DE_dec_A,       0x17, X_NONE )
    INSN  ( "cmpbkc",  DE_inc_HL_inc,  0x27, X_NONE )
    INSN  ( "cmpbkc",  DE_dec_HL_dec,  0x37, X_NONE )
    
    INSN  ( "cmpmnc",  DE_inc_A,       0x06, X_NONE )
    INSN  ( "cmpmnc",  DE_dec_A,       0x16, X_NONE )
    INSN  ( "cmpbknc", DE_inc_HL_inc,  0x26, X_NONE )
    INSN  ( "cmpbknc", DE_dec_HL_dec,  0x36, X_NONE )

    END
};

/******************************************************************************/

optab_t base_optab[] = {

/*----------------------------------------------------------------------------
  Data Transfer
  ----------------------------------------------------------------------------*/

    RANGE ( "mov",  r1_byte,       0xB8, 0xBF, X_NONE )
    INSN  ( "mov",  saddr_byte,    0x3A, X_NONE )
    INSN  ( "mov",  sfr_byte,      0x2B, X_NONE )
    MASK2 ( "mov",  r_r1,          0x24, 0x08, 0x00, X_NONE )
    RANGE ( "mov",  A_r1,          0xD0, 0xD7, X_NONE )
    INSN  ( "mov",  A_saddr,       0x20, X_NONE )
    INSN  ( "mov",  saddr_A,       0x22, X_NONE )
    INSN  ( "mov",  saddr_saddr,   0x38, X_NONE )
    INSN  ( "mov",  A_sfr,         0x10, X_NONE )
    INSN  ( "mov",  sfr_A,         0x12, X_NONE )
    RANGE ( "mov",  A_mem,         0x58, 0x5D, X_NONE )
    MEMMOD( "mov",  A_memmod,      0x00, X_NONE ) 
    RANGE ( "mov",  mem_A,         0x50, 0x55, X_NONE )
    MEMMOD( "mov",  memmod_A,      0x80, X_NONE )
    INSN  ( "mov",  A_saddrp,      0x18, X_NONE )
    INSN  ( "mov",  saddrp_A,      0x19, X_NONE )
    
    /* ----------------------------------------------- */

    RANGE ( "xch",  A_r1,          0xD8, 0xDF, X_NONE )
    MASK2 ( "xch",  r_r1,          0x25, 0x08, 0x00, X_NONE )
    MEMMOD( "xch",  A_memmod,      0x04, X_NONE )
    INSN  ( "xch",  A_saddr,       0x21, X_NONE )
    INSN  ( "xch",  A_saddrp,      0x23, X_NONE )
    INSN  ( "xch",  saddr_saddr,   0x39, X_NONE )
    
    /* ----------------------------------------------- */
    
    RANGE ( "movw", rp1_word,       0x60, 0x67, X_NONE )
    INSN  ( "movw", saddrp_word,   0x0C, X_NONE )
    INSN  ( "movw", sfrp_word,     0x0B, X_NONE )
    MASK2 ( "movw", rp_rp1,        0x24, 0x08, 0x08, X_NONE )
    INSN  ( "movw", AX_saddrp,     0x1C, X_NONE )
    INSN  ( "movw", saddrp_AX,     0x1A, X_NONE )
    INSN  ( "movw", saddrp_saddrp, 0x3C, X_NONE )
    INSN  ( "movw", AX_sfrp,       0x11, X_NONE )
    INSN  ( "movw", sfrp_AX,       0x13, X_NONE )
    
    /* ----------------------------------------------- */
    
    INSN  ( "xchw", AX_saddrp,     0x1B, X_NONE )
    INSN  ( "xchw", saddrp_saddrp, 0x2A, X_NONE )
    MASK2 ( "xchw", rp_rp1,        0x25, 0x08, 0x08, X_NONE )

/*----------------------------------------------------------------------------
  8-Bit Operations
  ----------------------------------------------------------------------------*/
  
  /* These all follow a pattern so we can use a macro here to do the work for
   * us.
    */
  
#undef BYTE_OP
#define BYTE_OP(M_name, M_code)    INSN  ( M_name,  A_byte,        ( 0xA0 | M_code ), X_NONE ) \
                                            INSN  ( M_name,  saddr_byte,    ( 0x60 | M_code ), X_NONE ) \
                                            MASK2 ( M_name,  r_r1,          ( 0x80 | M_code ), 0x08, 0x00, X_NONE ) \
                                            INSN  ( M_name,  A_saddr,       ( 0x90 | M_code ), X_NONE ) \
                                            INSN  ( M_name,  saddr_saddr,   ( 0x70 | M_code ), X_NONE ) \
                                            MEMMOD( M_name,  A_memmod,      ( 0x00 | M_code ), X_NONE ) \
                                            MEMMOD( M_name,  memmod_A,      ( 0x80 | M_code ), X_NONE )
                                            
    BYTE_OP( "add",  0x08 )
    BYTE_OP( "addc", 0x09 )
    BYTE_OP( "sub",  0x0A )
    BYTE_OP( "subc", 0x0B )
    BYTE_OP( "and",  0x0C )
    BYTE_OP( "or",   0x0E )
    BYTE_OP( "xor",  0x0D )
    BYTE_OP( "cmp",  0x0F )
    
/*----------------------------------------------------------------------------
  16-Bit Operations
  ----------------------------------------------------------------------------*/

   /* These mostly follow a pattern so we can use a macro here to do the work
     * for us.
     */
    
#undef WORD_OP
#define WORD_OP(M_name, M_code)    INSN  ( M_name, AX_word,       ( 0x20 | M_code ), X_NONE ) \
                                            INSN  ( M_name, saddrp_word,   ( 0x00 | M_code ), X_NONE ) \
                                            INSN  ( M_name, AX_saddrp,     ( 0x10 | M_code ), X_NONE ) \
                                            INSN  ( M_name, saddrp_saddrp, ( 0x30 | M_code ), X_NONE )

    WORD_OP( "addw", 0x0D )
    WORD_OP( "subw", 0x0E )
    WORD_OP( "cmpw", 0x0F )
    
    /* Some ops don't fit the pattern */    
    MASK2 ( "addw", rp_rp1,        0x88, 0x08, 0x08, X_NONE )
    MASK2 ( "subw", rp_rp1,        0x8A, 0x08, 0x08, X_NONE )
    MASK2 ( "cmpw", rp_rp1,        0x8F, 0x08, 0x08, X_NONE )
  
/*----------------------------------------------------------------------------
  Multiplication/Division
  ----------------------------------------------------------------------------*/
  
    /* See optab_05 */

/*----------------------------------------------------------------------------
  Increment/Decrement
  ----------------------------------------------------------------------------*/    
    
    RANGE ( "inc",  r1,         0xC0, 0xC7, X_NONE )
    INSN  ( "inc",  saddr,      0x26, X_NONE )
    
    RANGE ( "dec",  r1,         0xC8, 0xCF, X_NONE )
    INSN  ( "dec",  saddr,      0x27, X_NONE )    
    
    RANGE ( "incw", rp2,        0x44, 0x47, X_NONE )
    /* incw saddrp in optab_03 */
    
    RANGE ( "decw", rp2,        0x4C, 0x4F, X_NONE )
    /* decw saddrp in optab_03 */

/*----------------------------------------------------------------------------
  Shift and Rotate
  ----------------------------------------------------------------------------*/    
    
    MASK2 ( "ror",  r1_n,    0x30, 0xC0, 0x40, X_NONE )
    MASK2 ( "rol",  r1_n,    0x31, 0xC0, 0x40, X_NONE )
    MASK2 ( "rorc", r1_n,    0x30, 0xC0, 0x00, X_NONE )
    MASK2 ( "rolc", r1_n,    0x31, 0xC0, 0x00, X_NONE )
    
    MASK2 ( "shr",  r1_n,    0x30, 0xC0, 0x80, X_NONE )
    MASK2 ( "shl",  r1_n,    0x31, 0xC0, 0x80, X_NONE )
    MASK2 ( "shrw", rp1_n,   0x30, 0xC0, 0xC0, X_NONE )
    MASK2 ( "shlw", rp1_n,   0x31, 0xC0, 0xC0, X_NONE )
    
    /* ror4 and rol4 in optab_05 */
    
/*----------------------------------------------------------------------------
  BCD Adjustment
  ----------------------------------------------------------------------------*/
  
   INSN  ( "adj4", none, 0x04, X_NONE )
    
/*----------------------------------------------------------------------------
  Bit Manipulation
  ----------------------------------------------------------------------------*/    
 
    /* See tables optab_02, optab_03 and optab_08 */
    
    RANGE ( "set1", saddr_bit,  0xB0, 0xB7, X_NONE )
    RANGE ( "clr1", saddr_bit,  0xA0, 0xA7, X_NONE)
    
    INSN  ( "set1", CY,         0x41, X_NONE )
    INSN  ( "clr1", CY,         0x40, X_NONE )
    INSN  ( "not1", CY,         0x42, X_NONE )
    
/*----------------------------------------------------------------------------
  Call/Return
  ----------------------------------------------------------------------------*/
  
    INSN  ( "call",   addr16,      0x28, X_CALL )
    RANGE ( "callf",  addr11,      0x90, 0x97, X_CALL )
    RANGE ( "callt",  ind_addr5,   0xE0, 0xFF, X_TABLE )
    INSN  ( "brk",    none,        0x5E, X_NONE )
    INSN  ( "ret",    none,        0x56, X_NONE )
    INSN  ( "reti",   none,        0x57, X_NONE )
    
/*----------------------------------------------------------------------------
  Stack Manipulation
  ----------------------------------------------------------------------------*/    
    
    INSN  ( "push",   post,       0x35, X_NONE )
    INSN  ( "push",   PSW,        0x49, X_NONE )
    INSN  ( "pushu",  post,       0x37, X_NONE )
    INSN  ( "pop",    post,       0x34, X_NONE )
    INSN  ( "pop",    PSW,        0x48, X_NONE )
    INSN  ( "popu",   post,       0x36, X_NONE )
    
/*----------------------------------------------------------------------------
  Unconditional Branch
  ----------------------------------------------------------------------------*/    
    
    INSN  ( "br",     addr16,     0x2C, X_JMP )
    INSN  ( "br",     addr16_rel, 0x14, X_JMP )
    
/*----------------------------------------------------------------------------
  Conditional Branch
  ----------------------------------------------------------------------------*/    
    
    INSN  ( "bc",     addr16_rel, 0x83, X_JMP )
    INSN  ( "bnc",    addr16_rel, 0x82, X_JMP )
    INSN  ( "bz",     addr16_rel, 0x81, X_JMP )
    INSN  ( "bnz",    addr16_rel, 0x80, X_JMP )
    INSN  ( "bv",     addr16_rel, 0x85, X_JMP )
    INSN  ( "bnv",    addr16_rel, 0x84, X_JMP )
    INSN  ( "bn",     addr16_rel, 0x87, X_JMP )
    INSN  ( "bp",     addr16_rel, 0x86, X_JMP )
    
   /* Bit test */
    
    RANGE ( "bt",     saddr_bit_addr16_rel,  0x70, 0x77, X_JMP )
    RANGE ( "dbnz",   r2_addr16_rel,         0x32, 0x33, X_JMP )
    INSN  ( "dbnz",   saddr_addr16_rel,      0x3B, X_JMP )
    
/*----------------------------------------------------------------------------
  Context Switch
  ----------------------------------------------------------------------------*/    
    
    /* BRKCS in optab_05 */
    INSN  ( "retcs",  addr16,  0x29, X_NONE )
    
/*----------------------------------------------------------------------------
  String
  ----------------------------------------------------------------------------*/    
    
    /* See optab_15 */
    
/*----------------------------------------------------------------------------
  CPU Control
  ----------------------------------------------------------------------------*/
    
    INSN  ( "swrs",   none,    0x43, X_NONE )
    INSN  ( "nop",    none,    0x00, X_NONE )
    INSN  ( "ei",     none,    0x4B, X_NONE )
    INSN  ( "di",     none,    0x4A, X_NONE )
    
/*----------------------------------------------------------------------------
  Instruction Sub-Group Tables
  ----------------------------------------------------------------------------*/
    
    TABLE ( optab_01, 0x01 )
    TABLE ( optab_02, 0x02 )
    TABLE ( optab_03, 0x03 )
    TABLE ( optab_05, 0x05 )
    TABLE ( optab_07, 0x07 )
    TABLE ( optab_08, 0x08 )
    TABLE ( optab_09, 0x09 )
    TABLE ( optab_15, 0x15 )

    END
};

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
