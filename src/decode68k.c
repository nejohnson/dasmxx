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
 *   68000 INSTRUCTION SET (upto and including the 68030, i.e. no FP)
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

DASM_PROFILE( "dasm68k", "Motorola 68000", 22, 9, 1, 2 )

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
    WORD imm = (WORD)nextw( f, addr );
    imm = ( imm << 16 ) | (UWORD)nextw( f, addr );
    
    operand( "%s#" FORMAT_IMM32, imm < 0 ? "-" : "", abs(imm) );
}

/************************************************************
 * Process variable-length relative address
 ************************************************************/
OPERAND_FUNC(relX)
{
    BYTE disp8 = opc & 0xFF;
    WORD dest = (ADDR)disp8;
    
    if ( disp8 == 0xFF || disp8 == 0 ) /* extended displacement */
    {
        dest = (WORD)nextw( f, addr );
        if ( disp8 == 0xFF ) /* 32-bit displacement */
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

OPERAND_FUNC(ea)
{
    int mode = opc & 0x7;
    int reg  = ( opc >> 3 ) & 0x7;
    
    switch( mode )
    {
    case EAMODE_DATA_DIRECT:                        /* 2.2.1 */
        operand( FORMAT_DREG, reg );
        break;
        
    case EAMODE_ADDR_DIRECT:                        /* 2.2.2 */
        operand( FORMAT_AREG, reg );
        break;
        
    case EAMODE_ADDR_INDIR:                         /* 2.2.3 */
        operand( "(" FORMAT_ADDR ")", reg );
        break;
        
    case EAMODE_ADDR_POST_INC:                      /* 2.2.4 */
        operand( "(" FORMAT_ADDR ")+", reg );
        break;
        
    case EAMODE_ADDR_PRE_DEC:                       /* 2.2.5 */
        operand( "-(" FORMAT_ADDR ")", reg );
        break;
        
    case EAMODE_ADDR_IND_DISP:                      /* 2.2.6 */
    {
        WORD disp = (WORD)nextw( f, addr );
        operand( "(" "%s#" FORMAT_IMM16 "," FORMAT_ADDR ")", 
                disp < 0 ? "-" : "", abs(disp), 
                reg );
        break;
    }
    
    case EAMODE_ADDR_IND_IDX:                       /* 2.2.7 - 2.2.10 */
    {
        UWORD extn = nextw( f, addr );
        int da     = extn & (1 << 15);
        int ireg   = ( extn >> 12 ) & 0x07;
        int wl     = extn & (1 << 11);
        int scale  = ( extn >> 9 ) & 0x03;
            
        if ( extn & 0x0100 ) /* check extension format bit */
        {
            /* 1 = full extension word format */
            int bs      = extn & (1 << 7);      /* base register suppress */
            int is      = extn & (1 << 6);      /* index register suppress */
            int bd_size = (extn >> 4) & 0x03;   /* base displacement size */
            int iis     = extn & 0x07;
            LWORD bd    = 0;                    /* base displacement */
            LWORD od    = 0;                    /* outer displacement */
            int od_size = iis & 0x03;           /* outer displacement size */
            int isiis   = ( is << 1 ) | ( iis >> 2 );
            
            /* Gather base displacement from insn stream */
            if ( bd_size == 0x02 || bd_size == 0x03 )
            {
                bd = (LWORD)nextw( f, addr );
                if ( bd_size == 0x03 )
                    bd = bd + ((LWORD)nextw( f, addr ) << 16);
            }
            
            /* Gather outer displacement from insn stream */
            if ( od_size == 0x02 || od_size == 0x03 ) {
                od = (LWORD)nextw( f, addr );
                if ( od_size == 0x03 )
                    od = od + ((LWORD)nextw( f, addr ) << 16);
            }
            
            operand( "( " );
            
            if ( isiis == 1 || isiis == 2 )
                operand( "[ " );
            
            operand( "%s" FORMAT_IMM32, bd < 0 ? "-" : "", abs(bd) );
            if ( !bs )
                operand( ", " FORMAT_AREG, reg );
                
            if ( isiis == 1 )
                operand( "], " );
                
            if ( !is )
            {
                operand( "%c%d.%c*%d" ")", 
                    da ? 'A' : 'D', ireg, wl ? 'L' : 'W', (1 << scale)
                );            
            }
            
            if ( isiis == 2 )
                operand( "]" );
            
            operand( ", %s" FORMAT_IMM32, od < 0 ? "-" : "", abs(od) );
            
            operand( " )" );
        }
        else
        {
            /* 0 = brief extension word format */
            WORD disp = extn & 0x00FF;
            
            if ( disp > 0x7F )
                disp -= 0x100;
                
            operand( "(%d," FORMAT_AREG "," "%c%d.%c*%d" ")", 
                disp, 
                reg, 
                da ? 'A' : 'D', ireg, wl ? 'L' : 'W', (1 << scale)
            );            
        }
    }
    
    case EAMODE_SUB_MODE:
    {
        
        
        
    }
    
    default:
        break;
    
    }
}






















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




/*----------------------------------------------------------------------------
  0001 - MOVE byte
  ----------------------------------------------------------------------------*/

    

/*----------------------------------------------------------------------------
  0010 - MOVE long (32 bit)
  ----------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------
  0011 - MOVE word (16 bit)
  ----------------------------------------------------------------------------*/






/*----------------------------------------------------------------------------
  0100 - MISC
  ----------------------------------------------------------------------------*/







/*----------------------------------------------------------------------------
  0101 - ADDQ, SUBQ, Scc, DBcc, TRAPc
  ----------------------------------------------------------------------------*/






/*----------------------------------------------------------------------------
  0110 - Bcc/BSR/BRA
  ----------------------------------------------------------------------------*/


    MASK ( "BRA",       relX,   0xFF00, 0x6000, X_JMP )
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






/*----------------------------------------------------------------------------
  1001 - SUB, SUBX
  ----------------------------------------------------------------------------*/





/*----------------------------------------------------------------------------
  1011 - CMP, EOR
  ----------------------------------------------------------------------------*/






/*----------------------------------------------------------------------------
  1100 - AND, MUL, ABCD, EXG
  ----------------------------------------------------------------------------*/



    MASK ( "EXG",       dreg9_dreg0,    0xF1F8, 0xC140, X_REG )
    MASK ( "EXG",       areg9_areg0,    0xF1F8, 0xC148, X_REG )
    MASK ( "EXG",       dreg9_areg0,    0xF1F8, 0xC188, X_REG )




/*----------------------------------------------------------------------------
  1101 - ADD, ADDX
  ----------------------------------------------------------------------------*/


    
    
    
    
/*----------------------------------------------------------------------------
  1110 - Shift, Rotate, Bit Field
  ----------------------------------------------------------------------------*/

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
  
    MASK ( "UNLK",      areg0,          0xFFF8, 0x4E58, X_REG )
    
    MASK ( "LINK",      areg0_simm16,   0xFFF8, 0x4E50, X_REG )
    MASK ( "LINK",      areg0_simm32,   0xFFF8, 0x4808, X_REG )
    
    
    
    
    

    MASK ( "EXT.W",     dreg0,  0xFFF8, 0x4880, X_REG )
    MASK ( "EXT.L",     dreg0,  0xFFF8, 0x48C0, X_REG )
    MASK ( "EXTB.L",    dreg0,  0xFFF8, 0x49C0, X_REG )

  
    MASK ( "SWAP",      dreg0, 0xFFF8, 0x4840, X_REG )

  
  
    INSN ( "NOP",       none,   0x4E71,         X_NONE )
    
    INSN ( "RTR",       none,   0x4E77,         X_NONE )
    INSN ( "RTS",       none,   0x4E75,         X_NONE )

  
    MASK ( "BKPT",      vector3, 0xFFF8, 0x4848, X_NONE )
  
    INSN ( "ILLEGAL",   none,   0x4AFC,         X_NONE )
  
    INSN ( "RESET",     none,   0x4E70,         X_NONE )
    INSN ( "RTE",       none,   0x4E73,         X_NONE )
    
    INSN ( "STOP",      imm16,  0x4E72,         X_IMM  )
    
    MASK ( "TRAP",      vector4, 0xFFF0, 0x4E40, X_NONE )
    
    INSN ( "TRAPV",     none,   0x4E76,         X_NONE )


    INSN ( "PFLUSHA",   none,   0x2400,         X_NONE )



















    
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
