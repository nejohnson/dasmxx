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
 * Globally-visible decoder properties
 *****************************************************************************/

DASM_PROFILE( "dasm09", "Motorola 6809", 4, 9, 0, 1 )

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/

/* Common output formats */
#define FORMAT_NUM_8BIT         "$%02X"
#define FORMAT_NUM_16BIT        "$%04X"
#define FORMAT_REG              "R%d"

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
    UBYTE msb   = next( f, addr );
    UBYTE lsb   = next( f, addr );
    UWORD imm16 = MK_WORD( lsb, msb );

    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, imm16 ) );
    xref_addxref( xtype, g_insn_addr, imm16 );
}

/***********************************************************
 * Process "direct" operand.
 ************************************************************/

OPERAND_FUNC(direct)
{
    UBYTE a = next( f, addr );
    
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, (ADDR)a ) );
    xref_addxref( xtype, g_insn_addr, a );
}

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

/***********************************************************
 * Process "r1_r2" operands.
 ************************************************************/

OPERAND_FUNC(r1_r2)
{
    UBYTE postbyte = next( f, addr );
    int   src = ( postbyte >> 4 ) & 0x0F;
    int   dst =   postbyte        & 0x0F;
    
    static const char * rtab[] = {
        "D",
        "X",
        "Y",
        "U",
        "S",
        "PC",
        "???", "???", /* not used */
        "A",
        "B",
        "CCR",
        "DPR"
    };
    
    operand( "%s", rtab[dst] );
    COMMA;
    operand( "%s", rtab[src] );
}

/******************************************************************************/
/** Instruction Decoding Tables                                              **/
/** Note: tables are here as they refer to operand functions defined above.  **/
/******************************************************************************/

static optab_t page2[] = {

    INSN ( "LBRN", rel16, 0x21, X_JMP )
    INSN ( "LBHI", rel16, 0x22, X_JMP )
    INSN ( "LBLS", rel16, 0x23, X_JMP )
    
    INSN ( "LBCC", rel16, 0x24, X_JMP )
    INSN ( "LBCS", rel16, 0x25, X_JMP )
    /* Two alternate namings */
    INSN ( "LBHS", rel16, 0x24, X_JMP )
    INSN ( "LBLO", rel16, 0x25, X_JMP )
    
    INSN ( "LBNE", rel16, 0x26, X_JMP )
    INSN ( "LBEQ", rel16, 0x27, X_JMP )
    INSN ( "LBVC", rel16, 0x28, X_JMP )
    INSN ( "LBVS", rel16, 0x29, X_JMP )
    INSN ( "LBPL", rel16, 0x2A, X_JMP )
    INSN ( "LBMI", rel16, 0x2B, X_JMP )
    INSN ( "LBGE", rel16, 0x2C, X_JMP )
    INSN ( "LBLT", rel16, 0x2D, X_JMP )
    INSN ( "LBGT", rel16, 0x2E, X_JMP )
    INSN ( "LBLE", rel16, 0x2F, X_JMP )

    INSN ( "SWI2", none,  0x3F, X_NONE )
    
    INSN ( "CMPD", imm16, 0x83, X_NONE )
    INSN ( "CMPY", imm16, 0x8C, X_NONE )
    INSN ( "LDY",  imm16, 0x8E, X_NONE )
    
    INSN ( "CMPD", direct, 0x93, X_NONE )
    INSN ( "CMPY", direct, 0x9C, X_NONE )
    INSN ( "LDY",  direct, 0x9E, X_NONE )
    INSN ( "STY",  direct, 0x9F, X_NONE )
    
    INSN ( "CMPD", indexed, 0xA3, X_NONE )
    INSN ( "CMPY", indexed, 0xAC, X_NONE )
    INSN ( "LDY",  indexed, 0xAE, X_NONE )
    INSN ( "STY",  indexed, 0xAF, X_NONE )

    INSN ( "CMPD", extended, 0xB3, X_NONE )
    INSN ( "CMPY", extended, 0xBC, X_NONE )
    INSN ( "LDY",  extended, 0xBE, X_NONE )
    INSN ( "STY",  extended, 0xBF, X_NONE )
    
    INSN ( "LDS", imm16,  0xCE, X_NONE )
    INSN ( "LDS", direct, 0xDE, X_NONE )
    INSN ( "STS", direct, 0xDF, X_NONE )
    
    INSN ( "LDS", indexed, 0xEE, X_NONE )
    INSN ( "STS", indexed, 0xEF, X_NONE )

    INSN ( "LDS", extended, 0xFE, X_NONE )
    INSN ( "STS", extended, 0xFF, X_NONE )
    
    END
};

static optab_t page3[] = {

    INSN ( "SWI3", none,     0x3F, X_NONE )

    INSN ( "CMPU", imm16,    0x83, X_NONE )
    INSN ( "CMPS", imm16,    0x8C, X_NONE )

    INSN ( "CMPU", direct,   0x93, X_NONE )
    INSN ( "CMPS", direct,   0x9C, X_NONE )

    INSN ( "CMPU", indexed,  0xA3, X_NONE )
    INSN ( "CMPS", indexed,  0xAC, X_NONE )

    INSN ( "CMPU", extended, 0xB3, X_NONE )
    INSN ( "CMPS", extended, 0xBC, X_NONE )

    END
};

/** Macros to define classes of instruction **/

#define ACC_ARGS_OP(M_name, M_base)    \
        INSN(M_name, imm8,     (0x80 | M_base), X_NONE) \
        INSN(M_name, direct,   (0x90 | M_base), X_NONE) \
        INSN(M_name, indexed,  (0xA0 | M_base), X_NONE) \
        INSN(M_name, extended, (0xB0 | M_base), X_NONE)
        
#define ACC_ARGS_OPD(M_name, M_base)    \
        INSN(M_name, imm16,    (0xC0 | M_base), X_NONE) \
        INSN(M_name, direct,   (0xD0 | M_base), X_NONE) \
        INSN(M_name, indexed,  (0xE0 | M_base), X_NONE) \
        INSN(M_name, extended, (0xF0 | M_base), X_NONE)        

#define ACC_AB_ARGS_OP(M_name, M_base)    \
        ACC_ARGS_OP(M_name "A", (0x00 | M_base)) \
        ACC_ARGS_OP(M_name "B", (0x40 | M_base))

#define SINGLE_OP(M_name, M_base) \
        INSN(M_name, direct,   (0x00 | M_base), X_NONE) \
        INSN(M_name, indexed,  (0x60 | M_base), X_NONE) \
        INSN(M_name, extended, (0x70 | M_base), X_NONE)
        
#define ACC_ARGS_OP_NOIMM(M_name, M_base, M_xref)    \
        INSN(M_name, direct,   (0x90 | M_base), M_xref) \
        INSN(M_name, indexed,  (0xA0 | M_base), M_xref) \
        INSN(M_name, extended, (0xB0 | M_base), M_xref)
        
#define ACC_OP_INH(M_name, M_base) \
        INSN( M_name "A", none, ( 0x40 | M_base ), X_NONE ) \
        INSN( M_name "B", none, ( 0x50 | M_base ), X_NONE )            

optab_t base_optab[] = {

/*----------------------------------------------------------------------------
  8-bit Accumulator and Memory
  ----------------------------------------------------------------------------*/
  
    ACC_AB_ARGS_OP( "ADC", 0x09 )
    ACC_AB_ARGS_OP( "ADD", 0x0B )
    ACC_AB_ARGS_OP( "AND", 0x04 )
    ACC_AB_ARGS_OP( "BIT", 0x05 )
    ACC_AB_ARGS_OP( "CMP", 0x01 )
    ACC_AB_ARGS_OP( "EOR", 0x08 )
    ACC_AB_ARGS_OP( "LD",  0x06 )
    ACC_AB_ARGS_OP( "OR",  0x0A )
    ACC_AB_ARGS_OP( "SBC", 0x02 )
    ACC_AB_ARGS_OP( "SUB", 0x00 )
    
    ACC_ARGS_OP_NOIMM( "STA", 0x07, X_PTR )
    ACC_ARGS_OP_NOIMM( "STB", 0x47, X_PTR )
    
    SINGLE_OP( "ASL", 0x08 )
    SINGLE_OP( "ASR", 0x07 )
    SINGLE_OP( "CLR", 0x0F )
    SINGLE_OP( "COM", 0x03 )
    SINGLE_OP( "DEC", 0x0A )
    SINGLE_OP( "INC", 0x0C )
    SINGLE_OP( "LSL", 0x08 ) /* Note same opcodes as for ASLA/B */
    SINGLE_OP( "LSR", 0x04 )
    SINGLE_OP( "NEG", 0x00 )
    SINGLE_OP( "ROL", 0x09 )
    SINGLE_OP( "ROR", 0x06 )
    SINGLE_OP( "TST", 0x0D )
    
    ACC_OP_INH( "ASL", 0x08 )
    ACC_OP_INH( "ASR", 0x07 )
    ACC_OP_INH( "CLR", 0x0F )
    ACC_OP_INH( "COM", 0x03 )
    ACC_OP_INH( "DEC", 0x0A )
    ACC_OP_INH( "INC", 0x0C )
    ACC_OP_INH( "LSL", 0x08 ) /* Note same opcodes as for ASLA/B */
    ACC_OP_INH( "LSR", 0x04 )
    ACC_OP_INH( "NEG", 0x00 )
    ACC_OP_INH( "ROL", 0x09 )
    ACC_OP_INH( "ROR", 0x06 )
    ACC_OP_INH( "TST", 0x0D )
    
    INSN ( "ABX",  none, 0x3A, X_NONE )
    INSN ( "DAA",  none, 0x19, X_NONE )
    INSN ( "MUL",  none, 0x3D, X_NONE )
    INSN ( "LBRA", rel16, 0x16, X_JMP )
    INSN ( "LBSR", rel16, 0x17, X_JMP )
    
/*----------------------------------------------------------------------------
  16-bit Accumulator and Memory
  ----------------------------------------------------------------------------*/

    ACC_ARGS_OPD( "ADDD", 0x03 )
    ACC_ARGS_OPD( "LDD",  0x0C )

    INSN ( "SEX",  none, 0x1D, X_NONE )

    ACC_ARGS_OP_NOIMM( "STD", 0x4D, X_PTR )
    ACC_ARGS_OP( "SUBD", 0x03 )
  
/*----------------------------------------------------------------------------
  Index Register/Stack Pointer
  ----------------------------------------------------------------------------*/

    ACC_ARGS_OP( "CMPX", 0x0C )
    
    INSN ( "EXG",    r1_r2, 0x1E, X_NONE )
    INSN ( "LEAX", indexed, 0x30, X_NONE )
    INSN ( "LEAY", indexed, 0x31, X_NONE )
    INSN ( "LEAS", indexed, 0x32, X_NONE )
    INSN ( "LEAU", indexed, 0x33, X_NONE )
    
    ACC_ARGS_OPD( "LDU",  0x0E )
    ACC_ARGS_OP( "LDX",  0x0E )
  
    INSN ( "PSHS", imm8, 0x34, X_NONE )
    INSN ( "PULS", imm8, 0x35, X_NONE )
    INSN ( "PSHU", imm8, 0x36, X_NONE )
    INSN ( "PULU", imm8, 0x37, X_NONE )
  
    ACC_ARGS_OP_NOIMM( "STU", 0x4F, X_PTR )
    ACC_ARGS_OP_NOIMM( "STX", 0x0F, X_PTR )
  
    INSN ( "TFR",   r1_r2, 0x1F, X_NONE )

/*----------------------------------------------------------------------------
  Branch
  ----------------------------------------------------------------------------*/
    
    INSN ( "BHI", rel8, 0x22, X_JMP )
    INSN ( "BLS", rel8, 0x23, X_JMP )
    
    INSN ( "BCC", rel8, 0x24, X_JMP )
    INSN ( "BCS", rel8, 0x25, X_JMP )
    /* Two alternate namings */
    INSN ( "BHS", rel8, 0x24, X_JMP )
    INSN ( "BLO", rel8, 0x25, X_JMP )
    
    INSN ( "BNE", rel8, 0x26, X_JMP )
    INSN ( "BEQ", rel8, 0x27, X_JMP )
    INSN ( "BVC", rel8, 0x28, X_JMP )
    INSN ( "BVS", rel8, 0x29, X_JMP )
    INSN ( "BPL", rel8, 0x2A, X_JMP )
    INSN ( "BMI", rel8, 0x2B, X_JMP )
    INSN ( "BGE", rel8, 0x2C, X_JMP )
    INSN ( "BLT", rel8, 0x2D, X_JMP )
    INSN ( "BGT", rel8, 0x2E, X_JMP )
    INSN ( "BLE", rel8, 0x2F, X_JMP )
  
    INSN ( "BSR", rel8, 0x8D, X_JMP )
    INSN ( "BRA", rel8, 0x20, X_JMP )
    INSN ( "BRN", rel8, 0x21, X_JMP )
    
/*----------------------------------------------------------------------------
  Miscellaneous
  ----------------------------------------------------------------------------*/
  
    INSN ( "ANDCC", imm8, 0x1C, X_NONE )
    INSN ( "CWAI",  imm8, 0x3C, X_NONE )
    INSN ( "NOP",   none, 0x12, X_NONE )
    INSN ( "ORCC",  imm8, 0x1A, X_NONE )
    INSN ( "RTI",   none, 0x3B, X_NONE )
    INSN ( "RTS",   none, 0x39, X_NONE )
    INSN ( "SWI",   none, 0x3F, X_NONE )
    INSN ( "SYNC",  none, 0x13, X_NONE )

    SINGLE_OP( "JMP", 0x0E )
    ACC_ARGS_OP_NOIMM( "JSR", 0x0D, X_CALL )
    
/*----------------------------------------------------------------------------
  Sub-Pages
  ----------------------------------------------------------------------------*/  
  
    TABLE ( page2, 0x10 )
    TABLE ( page3, 0x11 )
  
/*----------------------------------------------------------------------------*/  
    
    END
};

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
