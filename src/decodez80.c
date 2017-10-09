/*****************************************************************************
 *
 * Copyright (C) 2014-2016, Neil Johnson
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
 *   Z80 INSTRUCTION SET (as described in Zilog document UM008008-0116)
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

DASM_PROFILE( "dasmz80", "Zilog Z80", 4, 9, 0, 1 )

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

/******************************************************************************/
/**                            Single Operands                               **/
/******************************************************************************/

OPERAND_FUNC(a)
{
    operand( "A" );
}

OPERAND_FUNC(b)
{
    operand( "B" );
}

OPERAND_FUNC(c)
{
    operand( "C" );
}

OPERAND_FUNC(ind_c)
{
    operand( "(C)" );
}

OPERAND_FUNC(d)
{
    operand( "D" );
}

OPERAND_FUNC(e)
{
    operand( "E" );
}

OPERAND_FUNC(h)
{
    operand( "H" );
}

OPERAND_FUNC(l)
{
    operand( "L" );
}

OPERAND_FUNC(de)
{
    operand( "DE" );
}

OPERAND_FUNC(hl)
{
    operand( "HL" );
}

OPERAND_FUNC(ind_hl)
{
    operand( "(HL)" );
}

OPERAND_FUNC(af)
{
    operand( "AF" );
}

OPERAND_FUNC(afp)
{
    operand( "AF\'" );
}

OPERAND_FUNC(sp)
{
    operand( "SP" );
}

OPERAND_FUNC(indsp)
{
    operand( "(SP)" );
}

OPERAND_FUNC(ix)
{
    operand( "IX" );
}

OPERAND_FUNC(indix)
{
    operand( "(IX)" );
}

OPERAND_FUNC(ixl)
{
    operand( "IXL" );
}

OPERAND_FUNC(ixh)
{
    operand( "IXH" );
}

OPERAND_FUNC(ixX)
{
    if ( opc & 0x01 )
        operand_ixl( f, addr, opc, xtype );
    else
        operand_ixh( f, addr, opc, xtype );
}

OPERAND_FUNC(iy)
{
    operand( "IY" );
}

OPERAND_FUNC(indiy)
{
    operand( "(IY)" );
}

OPERAND_FUNC(iyl)
{
    operand( "IYL" );
}

OPERAND_FUNC(iyh)
{
    operand( "IYH" );
}

OPERAND_FUNC(iyX)
{
    if ( opc & 0x01 )
        operand_iyl( f, addr, opc, xtype );
    else
        operand_iyh( f, addr, opc, xtype );
}

OPERAND_FUNC(i)
{
    operand( "I" );
}

OPERAND_FUNC(r)
{
    operand( "R" );
}

OPERAND_FUNC(0)
{
    operand( "0" );
}

OPERAND_FUNC(1)
{
    operand( "1" );
}

OPERAND_FUNC(2)
{
    operand( "2" );
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

/***********************************************************
 * Process "imm16" operand.
 ************************************************************/

OPERAND_FUNC(imm16)
{
    UBYTE lsb   = next( f, addr );
    UBYTE msb   = next( f, addr );
    UWORD imm16 = MK_WORD( lsb, msb );

    operand( xref_genwordaddr( NULL, "#" FORMAT_NUM_16BIT, imm16 ) );
    xref_addxref( xtype, g_insn_addr, imm16 );
}

/***********************************************************
 * Bit operand within opcode
 ************************************************************/
 
/* xxBB_Bxxx */
OPERAND_FUNC(bit)
{
    UBYTE bit = ( opc >> 3 ) & 0x07;
    
    operand( "%d", bit );
}

/***********************************************************
 * Register operand within opcode
 ************************************************************/

/* xxxx_xRRR */
OPERAND_FUNC(reg)
{
    UBYTE reg = opc & 0x07;
    static char *rtab[] = { "B", "C", "D", "E", "H", "L", "(HL)", "A" };
    
    operand( "%s", rtab[reg] );
}

/* xxRRR_Rxxx */
OPERAND_FUNC(reg2)
{
    operand_reg( f, addr, opc >> 3, xtype );
}

/* xxRR_xxxx */
OPERAND_FUNC(rpair)
{
    UBYTE reg = ( opc >> 4 ) & 0x03;
    static char *rtab[] = { "BC", "DE", "HL", "SP" };
    
    operand( "%s", rtab[reg] );
}

/* xxRR_xxxx */
OPERAND_FUNC(indrpair)
{
    UBYTE reg = ( opc >> 4 ) & 0x03;
    static char *rtab[] = { "BC", "DE", "HL", "SP" };
    
    operand( "(%s)", rtab[reg] );
}

/***********************************************************
 * Process relative address operands.
 ************************************************************/

OPERAND_FUNC(rel8)
{
    BYTE disp = (BYTE)next( f, addr );
    ADDR dest = *addr + disp;
    
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

/***********************************************************
 * Process absolute memory address operands.
 ************************************************************/

OPERAND_FUNC(addr16)
{
    UBYTE lsb = next( f, addr );
    UBYTE msb = next( f, addr );
    ADDR dest = MK_WORD( lsb, msb );
    
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

/***********************************************************
 * Process absolute memory indirect address operands.
 ************************************************************/

OPERAND_FUNC(mem16)
{
    UBYTE lsb = next( f, addr );
    UBYTE msb = next( f, addr );
    ADDR dest = MK_WORD( lsb, msb );
    
    operand( "(%s)", xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

OPERAND_FUNC(mem8)
{
    UBYTE ioport = next( f, addr );
    
    operand( "(%s)", xref_genwordaddr( NULL, FORMAT_NUM_8BIT, ioport ) );
    xref_addxref( xtype, g_insn_addr, ioport );
}

/***********************************************************
 * Process condition code operands.
 ************************************************************/

/* xxCC_Cxxx */
OPERAND_FUNC(cond)
{
    UBYTE cond = ( opc >> 3 ) & 0x07;
    static char *ctab[] = { "NZ", "Z", "NC", "C", "PO", "PE", "P", "M" };

    operand( "%s", ctab[cond] );
}

/***********************************************************
 * Process RST (restart) operands.
 ************************************************************/

OPERAND_FUNC(rst)
{
    UBYTE rst = ( opc & 0x30 ) | ( ( opc & 0x0F ) == 0x0F ? 0x08 : 0x00 );
    
    operand( FORMAT_NUM_8BIT, rst );
}

/***********************************************************
 * Process IX/IY plus offset operands (IX + DISP)
 ************************************************************/
 
static void z80_emit_signed_index_offset( const char *idx, BYTE disp )
{
    if ( disp < 0 )
    	operand( "(%s-" FORMAT_NUM_8BIT ")", idx, -disp );
    else
    	operand( "(%s+" FORMAT_NUM_8BIT ")", idx, disp );
}

OPERAND_FUNC(ixoff)
{
    BYTE disp = (BYTE)next( f, addr );
    
    z80_emit_signed_index_offset( "IX", disp );
}

OPERAND_FUNC(iyoff)
{
    BYTE disp = (BYTE)next( f, addr );
    
    z80_emit_signed_index_offset( "IY", disp );
}

/***********************************************************
 * Process special IX/IY offset operands where the displacement
 * comes from the special operand stack.
 ************************************************************/

OPERAND_FUNC(ixoffS)
{
    BYTE disp = (BYTE)stack_pop();

    z80_emit_signed_index_offset( "IX", disp );    
}

OPERAND_FUNC(iyoffS)
{
    BYTE disp = (BYTE)stack_pop();

    z80_emit_signed_index_offset( "IY", disp );    
}

/******************************************************************************/
/**                            Double Operands                               **/
/******************************************************************************/

/*** ACC ***/

TWO_OPERAND(af, afp)
TWO_OPERAND(a, reg)
TWO_OPERAND(a, imm8)
TWO_OPERAND_PAIR(a, indrpair)
TWO_OPERAND_PAIR(a, mem16)
TWO_OPERAND_PAIR(a, mem8)
TWO_OPERAND_PAIR(a, i)
TWO_OPERAND_PAIR(a, r)

/*** 8-bit REG ***/

TWO_OPERAND(bit, reg)
TWO_OPERAND(reg2, imm8)
TWO_OPERAND_PAIR(reg2, mem8)
TWO_OPERAND_PAIR(reg2, ind_c)

/*** 16-bit REG */

TWO_OPERAND(rpair, imm16)
TWO_OPERAND(hl, rpair)
TWO_OPERAND(cond, addr16)
TWO_OPERAND(de, hl)
TWO_OPERAND(sp, hl)
TWO_OPERAND_PAIR(hl, mem16)
TWO_OPERAND_PAIR(rpair, mem16)
TWO_OPERAND(indsp, hl)

/*** IX ***/

TWO_OPERAND(ix, imm16)
TWO_OPERAND(ixoffS, reg)
TWO_OPERAND(bit, ixoffS)
TWO_OPERAND(ixoff, imm8)
TWO_OPERAND(ixoff, reg)
TWO_OPERAND(ix, ix)
TWO_OPERAND(ix, rpair)
TWO_OPERAND_PAIR(ix, mem16)
TWO_OPERAND(ixl, imm8)
TWO_OPERAND(ixh, imm8)
TWO_OPERAND(ixl, reg)
TWO_OPERAND(ixh, reg)
TWO_OPERAND(ixl, ixX)
TWO_OPERAND(ixh, ixX)
TWO_OPERAND(reg2, ixX)
TWO_OPERAND(a, ixX)
TWO_OPERAND(a, ixoff)
TWO_OPERAND(reg2, ixoff)
TWO_OPERAND(sp, ix)
TWO_OPERAND(indsp, ix)

/*** IY ***/

TWO_OPERAND(iy, imm16)
TWO_OPERAND(iyoffS, reg)
TWO_OPERAND(bit, iyoffS)
TWO_OPERAND(iyoff, imm8)
TWO_OPERAND(iyoff, reg)
TWO_OPERAND(iy, iy)
TWO_OPERAND(iy, rpair)
TWO_OPERAND(mem16, iy)
TWO_OPERAND(iy, mem16)
TWO_OPERAND(iyl, imm8)
TWO_OPERAND(iyh, imm8)
TWO_OPERAND(iyl, reg)
TWO_OPERAND(iyh, reg)
TWO_OPERAND(iyl, iyX)
TWO_OPERAND(iyh, iyX)
TWO_OPERAND(reg2, iyX)
TWO_OPERAND(a, iyX)
TWO_OPERAND(a, iyoff)
TWO_OPERAND(reg2, iyoff)
TWO_OPERAND(sp, iy)
TWO_OPERAND(indsp, iy)

/* Special double ops */

OPERAND_FUNC(rD_rS)
{
    operand_reg( f, addr, opc >> 3, xtype );
    COMMA;
    operand_reg( f, addr, opc, xtype );    
}

OPERAND_FUNC(condalt_rel8)
{
   operand_cond( f, addr, opc & ~0x20, xtype );
   COMMA;
   operand_rel8( f, addr, opc, xtype );
}

/******************************************************************************/
/**                            Triple Operands                               **/
/******************************************************************************/

THREE_OPERAND(bit, ixoffS, reg)
THREE_OPERAND(bit, iyoffS, reg)

/******************************************************************************/
/** Instruction Decoding Tables                                              **/
/** Note: tables are here as they refer to operand functions defined above.  **/
/******************************************************************************/

/******************************************************************************/
/* Common bit operations */
/******************************************************************************/

optab_t pageBITS[] = {

    MASK ( "RLC", reg, 0xF8, 0x00, X_REG )
    MASK ( "RRC", reg, 0xF8, 0x08, X_REG )
    MASK ( "RL",  reg, 0xF8, 0x10, X_REG )
    MASK ( "RR",  reg, 0xF8, 0x18, X_REG )
    
    MASK ( "SLA", reg, 0xF8, 0x20, X_REG )
    MASK ( "SRA", reg, 0xF8, 0x28, X_REG )
    MASK ( "SRL", reg, 0xF8, 0x38, X_REG )
    
    MASK ( "BIT", bit_reg, 0xC0, 0x40, X_REG )
    MASK ( "RES", bit_reg, 0xC0, 0x80, X_REG )
    MASK ( "SET", bit_reg, 0xC0, 0xC0, X_REG )
    
    END
};

/******************************************************************************/
/* Extended operations */
/******************************************************************************/

optab_t pageEXTD[] = {

    MASK ( "ADC", hl_rpair, 0xCF, 0x4A, X_NONE )
    MASK ( "SBC", hl_rpair, 0xCF, 0x42, X_NONE )
    INSN ( "NEG", none,     0x44, X_NONE )
    
    INSN ( "LDI", none, 0xA0, X_NONE )
    INSN ( "CPI", none, 0xA1, X_NONE )
    INSN ( "INI", none, 0xA2, X_NONE )
    INSN ( "OUTI", none, 0xA3, X_NONE )
    
    INSN ( "LDIR", none, 0xB0, X_NONE )
    INSN ( "CPIR", none, 0xB1, X_NONE )
    INSN ( "INIR", none, 0xB2, X_NONE )
    INSN ( "OTIR", none, 0xB3, X_NONE )
    
    INSN ( "RRD",  none, 0x67, X_NONE )
    INSN ( "RLD",  none, 0x6F, X_NONE )
    
    INSN ( "LDD",  none, 0xA8, X_NONE )
    INSN ( "CPD",  none, 0xA9, X_NONE )
    INSN ( "IND",  none, 0xAA, X_NONE )
    INSN ( "OUTD", none, 0xAB, X_NONE )
    INSN ( "LDDR", none, 0xB8, X_NONE )
    INSN ( "CPDR", none, 0xB9, X_NONE )
    INSN ( "INDR", none, 0xBA, X_NONE )
    INSN ( "OTDR", none, 0xBB, X_NONE )
    
    /* Order is important! */
    INSN ( "RETI", none, 0x4D, X_NONE )
    INSN ( "RETN", none, 0x45, X_NONE )
    
    MASK ( "LD",   mem16_rpair, 0xCF, 0x43, X_PTR )
    MASK ( "LD",   rpair_mem16, 0xCF, 0x4B, X_PTR )
    
    INSN ( "IN",   reg2_ind_c, 0x40, X_IO )
    INSN ( "IN",   reg2_ind_c, 0x50, X_IO )
    INSN ( "IN",   reg2_ind_c, 0x60, X_IO )
    MASK ( "IN",   reg2_ind_c, 0xCF, 0x48, X_IO )
    
    INSN ( "OUT",  ind_c_reg2, 0x41, X_IO )
    INSN ( "OUT",  ind_c_reg2, 0x51, X_IO )
    INSN ( "OUT",  ind_c_reg2, 0x61, X_IO )
    MASK ( "OUT",  ind_c_reg2, 0xCF, 0x49, X_IO )
    
    INSN ( "LD",   i_a, 0x47, X_NONE )
    INSN ( "LD",   a_i, 0x57, X_NONE )
    INSN ( "LD",   r_a, 0x4F, X_NONE )
    INSN ( "LD",   a_r, 0x5F, X_NONE )
    
    INSN ( "IM",   0, 0x46, X_NONE )
    INSN ( "IM",   1, 0x56, X_NONE )
    INSN ( "IM",   2, 0x5E, X_NONE )
 
    END
};

/******************************************************************************/
/* Index Register IX Operations */
/******************************************************************************/
  
optab_t pageIXBITS[] = {

    INSN ( "RLC", ixoffS,   0x06, X_REG )
    MASK ( "RLC", ixoffS_reg, 0xF8, 0x00, X_REG )
    
    INSN ( "RRC", ixoffS,   0x0E, X_REG )
    MASK ( "RRC", ixoffS_reg, 0xF8, 0x08, X_REG )
    
    INSN ( "RL",  ixoffS,   0x16, X_REG )
    MASK ( "RL",  ixoffS_reg, 0xF8, 0x10, X_REG )
    
    INSN ( "RR",  ixoffS,   0x1E, X_REG )
    MASK ( "RR",  ixoffS_reg, 0xF8, 0x18, X_REG )
    
    INSN ( "SLA", ixoffS,   0x26, X_REG )
    MASK ( "SLA", ixoffS_reg, 0xF8, 0x20, X_REG )
    
    INSN ( "SRA", ixoffS,   0x2E, X_REG )
    MASK ( "SRA", ixoffS_reg, 0xF8, 0x28, X_REG )
    
    INSN ( "SLL", ixoffS,   0x36, X_REG )
    MASK ( "SLL", ixoffS_reg, 0xF8, 0x30, X_REG )
    
    INSN ( "SRL", ixoffS,   0x3E, X_REG )
    MASK ( "SRL", ixoffS_reg, 0xF8, 0x38, X_REG )
    
    MASK ( "BIT", bit_ixoffS,     0xC0, 0x40, X_REG )
    
    MASK ( "RES", bit_ixoffS,     0xC7, 0x86, X_REG )
    MASK ( "RES", bit_ixoffS_reg, 0xC0, 0x80, X_REG )

    MASK ( "SET", bit_ixoffS,     0xC7, 0xC6, X_REG )
    MASK ( "SET", bit_ixoffS_reg, 0xC0, 0xC0, X_REG )
    
    END
};

optab_t pageIX[] = {

    INSN ( "LD", ix_imm16,   0x21, X_IMM )
    INSN ( "LD", mem16_ix,   0x22, X_DIRECT )
    INSN ( "LD", ix_mem16,   0x2A, X_DIRECT )
    INSN ( "LD", sp_ix,      0xF9, X_REG )
    
    RANGE( "LD", ixoff_reg,  0x70, 0x75, X_REG )
    INSN ( "LD", ixoff_reg,  0x77, X_REG )
    INSN ( "LD", ixoff_imm8, 0x36, X_IMM )
    
    UNDEF( 0x76 )
    MASK ( "LD", reg2_ixoff, 0xC7, 0x46, X_REG )    

    INSN ( "POP",  ix,       0xE1, X_REG )
    INSN ( "PUSH", ix,       0xE5, X_REG )
    
    INSN ( "EX",   indsp_ix, 0xE3, X_REG )
    
    INSN ( "INC",  ix,       0x23, X_REG )
    INSN ( "INC",  ixoff,    0x34, X_REG )
    
    INSN ( "DEC",  ix,       0x2B, X_REG )
    INSN ( "DEC",  ixoff,    0x35, X_REG )
    
    INSN ( "ADD",  ix_ix,    0x29, X_REG )
    MASK ( "ADD",  ix_rpair, 0xCF, 0x09, X_REG )
    INSN ( "ADD",  a_ixoff,  0x86, X_REG )
    INSN ( "ADC",  a_ixoff,  0x8E, X_REG )
    INSN ( "SUB",  a_ixoff,  0x96, X_REG )
    INSN ( "SBC",  a_ixoff,  0x9E, X_REG )
    INSN ( "AND",  a_ixoff,  0xA6, X_REG )
    INSN ( "XOR",  a_ixoff,  0xAE, X_REG )
    INSN ( "OR",   a_ixoff,  0xB6, X_REG )
    INSN ( "CP",   a_ixoff,  0xBE, X_REG )
    
    INSN ( "JP",   indix,    0xE9, X_REG )    

    PUSHTBL ( pageIXBITS,    0xCB, 1 )
    
    END
};

/******************************************************************************/
/* Index Register IY Operations */
/******************************************************************************/

optab_t pageIYBITS[] = {

    INSN ( "RLC", iyoffS,   0x06, X_REG )
    INSN ( "RRC", iyoffS,   0x0E, X_REG )
    INSN ( "RL",  iyoffS,   0x16, X_REG )
    INSN ( "RR",  iyoffS,   0x1E, X_REG )
    INSN ( "SLA", iyoffS,   0x26, X_REG )
    INSN ( "SRA", iyoffS,   0x2E, X_REG )
    INSN ( "SRL", iyoffS,   0x3E, X_REG )
    
    MASK ( "BIT", bit_iyoffS,     0xC0, 0x40, X_REG )
    
    MASK ( "RES", bit_iyoffS,     0xC7, 0x86, X_REG )
    MASK ( "RES", bit_iyoffS_reg, 0xC0, 0x80, X_REG )

    MASK ( "SET", bit_iyoffS,     0xC7, 0xC6, X_REG )
    MASK ( "SET", bit_iyoffS_reg, 0xC0, 0xC0, X_REG )
    
    END
};

optab_t pageIY[] = {
    
    INSN ( "LD", iy_imm16,   0x21, X_IMM )
    INSN ( "LD", mem16_iy,   0x22, X_DIRECT )
    INSN ( "LD", iy_mem16,   0x2A, X_DIRECT )
    INSN ( "LD", sp_iy,      0xF9, X_REG )
    
    RANGE( "LD", iyoff_reg,  0x70, 0x75, X_REG )
    INSN ( "LD", iyoff_reg,  0x77, X_REG )
    INSN ( "LD", iyoff_imm8, 0x36, X_IMM )
    
    UNDEF( 0x76 )
    MASK ( "LD", reg2_iyoff, 0xC7, 0x46, X_REG )    

    INSN ( "POP",  iy,       0xE1, X_REG )
    INSN ( "PUSH", iy,       0xE5, X_REG )
    
    INSN ( "EX",   indsp_iy, 0xE3, X_REG )
    
    INSN ( "INC",  iy,       0x23, X_REG )
    INSN ( "INC",  iyoff,    0x34, X_REG )
    
    INSN ( "DEC",  iy,       0x2B, X_REG )
    INSN ( "DEC",  iyoff,    0x35, X_REG )
    
    INSN ( "ADD",  iy_iy,    0x29, X_REG )
    MASK ( "ADD",  iy_rpair, 0xCF, 0x09, X_REG )
    INSN ( "ADD",  a_iyoff,  0x86, X_REG )
    INSN ( "ADC",  a_iyoff,  0x8E, X_REG )
    INSN ( "SUB",  a_iyoff,  0x96, X_REG )
    INSN ( "SBC",  a_iyoff,  0x9E, X_REG )
    INSN ( "AND",  a_iyoff,  0xA6, X_REG )
    INSN ( "XOR",  a_iyoff,  0xAE, X_REG )
    INSN ( "OR",   a_iyoff,  0xB6, X_REG )
    INSN ( "CP",   a_iyoff,  0xBE, X_REG )
    
    INSN ( "JP",   indiy,    0xE9, X_REG ) 
    
    PUSHTBL ( pageIYBITS, 0xCB, 1 )
    
    END
};

/******************************************************************************/
/** Base table **/
/******************************************************************************/

optab_t base_optab[] = {
    
    INSN ( "HALT", none, 0x76, X_NONE )
    
/*----------------------------------------------------------------------------
  Load/Store/Push/Pop
  ----------------------------------------------------------------------------*/
  
    MASK ( "LD", rD_rS,       0xC0, 0x40, X_PTR )
    MASK ( "LD", reg2_imm8,   0xC7, 0x06, X_IMM )    
    MASK ( "LD", rpair_imm16, 0xCF, 0x01, X_IMM )
    
    MASK ( "LD", indrpair_a,  0xEF, 0x02, X_PTR )
    MASK ( "LD", a_indrpair,  0xEF, 0x0A, X_PTR )
    
    INSN ( "LD", mem16_hl,    0x22, X_PTR )
    INSN ( "LD", hl_mem16,    0x2A, X_PTR )
    
    INSN ( "LD", mem16_a,     0x32, X_PTR )
    INSN ( "LD", a_mem16,     0x3A, X_PTR )    
        
    /* Order is important! */
    INSN ( "POP", af,    0xF1,       X_REG )
    MASK ( "POP", rpair, 0xCF, 0xC1, X_REG )
    
    /* Order is important! */
    INSN ( "PUSH", af,    0xF5,       X_REG )
    MASK ( "PUSH", rpair, 0xCF, 0xC5, X_REG )
    
    INSN ( "IN",   a_mem8, 0xDB, X_IO )
    INSN ( "OUT",  mem8_a, 0xD3, X_IO )
    
    INSN ( "LD",   sp_hl, 0xF9, X_NONE )

/*----------------------------------------------------------------------------
  8-bit Accumulator and Memory
  ----------------------------------------------------------------------------*/
  
    MASK ( "ADD", a_reg, 0xF8, 0x80, X_NONE )
    MASK ( "ADC", a_reg, 0xF8, 0x88, X_NONE )
    MASK ( "SUB", a_reg, 0xF8, 0x90, X_NONE )
    MASK ( "SBC", a_reg, 0xF8, 0x98, X_NONE )
    MASK ( "AND", a_reg, 0xF8, 0xA0, X_NONE )
    MASK ( "XOR", a_reg, 0xF8, 0xA8, X_NONE )
    MASK ( "OR",  a_reg, 0xF8, 0xB0, X_NONE )
    MASK ( "CP",  a_reg, 0xF8, 0xB8, X_NONE )
    
    INSN ( "ADD", a_imm8,  0xC6, X_IMM )
    INSN ( "ADC", a_imm8,  0xCE, X_IMM )
    INSN ( "SUB", a_imm8,  0xD6, X_IMM )
    INSN ( "SBC", a_imm8,  0xDE, X_IMM )
    INSN ( "AND", a_imm8,  0xE6, X_IMM )
    INSN ( "OR",  a_imm8,  0xF6, X_IMM )
    
    INSN ( "RLCA", none, 0x07, X_NONE )
    INSN ( "RRCA", none, 0x0F, X_NONE )
    INSN ( "RLA",  none, 0x17, X_NONE )
    INSN ( "RRA",  none, 0x1F, X_NONE )
    
    INSN ( "DAA",  none, 0x27, X_NONE )
    INSN ( "CPL",  none, 0x2F, X_NONE )
    INSN ( "SCF",  none, 0x37, X_NONE )
    INSN ( "CCF",  none, 0x3F, X_NONE )
    
    INSN ( "XOR",  imm8, 0xEE, X_IMM )
    INSN ( "CP",   imm8, 0xFE, X_IMM )
    
    MASK ( "INC",  reg2, 0xC7, 0x04, X_REG )
    MASK ( "DEC",  reg2, 0xC7, 0x05, X_REG )
    
/*----------------------------------------------------------------------------
  16-bit Accumulator and Memory
  ----------------------------------------------------------------------------*/

    MASK ( "INC", rpair,    0xCF, 0x03, X_NONE )
    MASK ( "DEC", rpair,    0xCF, 0x0B, X_NONE )
    MASK ( "ADD", hl_rpair, 0xCF, 0x09, X_NONE )

/*----------------------------------------------------------------------------
  Branch
  ----------------------------------------------------------------------------*/
    
    INSN ( "RET",  none,        0xC9,       X_NONE )
    MASK ( "RET",  cond,        0xC7, 0xC0, X_NONE )
    
    INSN ( "DJNZ", rel8,        0x10,       X_JMP )
    
    INSN ( "JP",   addr16,      0xC3,       X_JMP )
    MASK ( "JP",   cond_addr16, 0xC7, 0xC2, X_JMP )
    INSN ( "JP",   indrpair,    0xE9,       X_REG )
    
    INSN ( "CALL", addr16,      0xCD,       X_CALL )
    MASK ( "CALL", cond_addr16, 0xC7, 0xC4, X_CALL )

    INSN ( "JR",   rel8,        0x18,       X_JMP )
    MASK ( "JR",   condalt_rel8, 0xE7, 0x20, X_JMP )
    
/*----------------------------------------------------------------------------
  Miscellaneous
  ----------------------------------------------------------------------------*/
  
    INSN ( "NOP",   none, 0x00, X_NONE )
    
    INSN ( "DI",    none, 0xF3, X_NONE )
    INSN ( "EI",    none, 0xFB, X_NONE )
    
    INSN ( "EXX",   none, 0xD9, X_NONE )
    INSN ( "EX",    de_hl, 0xEB, X_NONE )
    INSN ( "EX",    af_afp, 0x08, X_NONE )
    INSN ( "EX",    indsp_hl, 0xE3, X_NONE )
    
    MASK ( "RST",   rst,  0xC7, 0xC7, X_NONE )
    
/*----------------------------------------------------------------------------
  Sub-Pages
  ----------------------------------------------------------------------------*/  

    TABLE ( pageBITS, 0xCB )
    TABLE ( pageIX,   0xDD )
    TABLE ( pageEXTD, 0xED )
    TABLE ( pageIY,   0xFD )
      
/*----------------------------------------------------------------------------*/  
    
    END
};

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
