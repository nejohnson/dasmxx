/*****************************************************************************
 *
 * Copyright (C) 2017-2024, Neil Johnson
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

DASM_PROFILE( "dasm48", "Intel MCS-48 (8035, 8048, 8049)", 4, 9, 0, 1, 1 )

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/

/* Common output formats */
#define FORMAT_NUM_8BIT      "0%02XH"
#define FORMAT_NUM_16BIT     "0%04XH"
#define FORMAT_REG           "R%d"
#define FORMAT_PORT          "P%d"

/* Construct a 16-bit word out of low and high bytes */
#define MK_WORD(l,h)         ( ((l) & 0xFF) | (((h) & 0xFF) << 8) )

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
 * Hard-coded registers.
 ************************************************************/

OPERAND_FUNC(A)
{
   operand( "A" );
}

OPERAND_FUNC(indA)
{
   operand( "@A" );
}

OPERAND_FUNC(I)
{
   operand( "I" );
}

OPERAND_FUNC(C)
{
   operand( "C" );
}

OPERAND_FUNC(T)
{
   operand( "T" );
}

OPERAND_FUNC(PSW)
{
   operand( "PSW" );
}

OPERAND_FUNC(BUS)
{
   operand( "BUS" );
}

OPERAND_FUNC(CLK)
{
   operand( "CLK" );
}

OPERAND_FUNC(CNT)
{
   operand( "CNT" );
}

OPERAND_FUNC(TCNT)
{
   operand( "TCNT" );
}

OPERAND_FUNC(TCNTI)
{
   operand( "TCNTI" );
}

OPERAND_FUNC(F0)
{
   operand( "F0" );
}

OPERAND_FUNC(F1)
{
   operand( "F1" );
}

OPERAND_FUNC(RB0)
{
   operand( "RB0" );
}

OPERAND_FUNC(RB1)
{
   operand( "RB1" );
}

OPERAND_FUNC(MB0)
{
   operand( "MB0" );
}

OPERAND_FUNC(MB1)
{
   operand( "MB1" );
}

/***********************************************************
 * General register.
 *   reg.num comes from bottom 3 bits of OPC.
 ************************************************************/

OPERAND_FUNC(reg)
{
   UBYTE reg = opc & 0x07;
   
   operand( FORMAT_REG, reg );
}

/***********************************************************
 * Port
 *   port num comes from bottom 2 bits of OPC.
 ************************************************************/

OPERAND_FUNC(port)
{
   UBYTE port = opc & 0x03;
   
   operand( FORMAT_PORT, port );
}

/***********************************************************
 * PortH
 *   high port num comes from 4 + bottom 2 bits of OPC.
 ************************************************************/

OPERAND_FUNC(portH)
{
   UBYTE port = ( opc & 0x03 ) + 4;
   
   operand( FORMAT_PORT, port );
}

/***********************************************************
 * Indirect register.
 *   reg.num is either 0 or 1 depending on the OPC.
 ************************************************************/

OPERAND_FUNC(indreg)
{
   UBYTE reg = opc & 0x01;
   
   operand( "@" FORMAT_REG, reg );
}

/***********************************************************
 * 8-bit Immediate operand
 *   value comes from next byte.
 ************************************************************/

OPERAND_FUNC(imm8)
{
   UBYTE imm8 = next( f, addr );
   
   operand( "#" FORMAT_NUM_8BIT, imm8 );
}

/***********************************************************
 * 8-bit Bit Address
 *   value comes from top three bits of OPC.
 ************************************************************/

OPERAND_FUNC(addrbit)
{
   UBYTE bit = ( opc >> 5 ) & 0x07;

   operand( "%d", bit );
}

/***********************************************************
 * 8-bit Address.
 *    Address comes from next byte.
 ************************************************************/

OPERAND_FUNC(addr8)
{
   UBYTE addr8 = (UBYTE)next( f, addr );
   
   operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr8 ) );
   xref_addxref( xtype, g_insn_addr, addr8 );
}

/***********************************************************
 * 11-bit address
 *   Address comes from top three bits of OPC and next byte.
 ************************************************************/
 
OPERAND_FUNC(addr11)
{
   UBYTE msb_addr  = ( opc >> 5) & 0x07;
   UBYTE lsb_addr  = next( f, addr );
   UWORD addr11    = MK_WORD( lsb_addr, msb_addr );

   operand( "%s", xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr11 ) );
   xref_addxref( xtype, g_insn_addr, addr11 );
}

/******************************************************************************/
/**                            Double Operands                               **/
/******************************************************************************/

/** Accumulator operands, some are pairs **/
TWO_OPERAND(A, indA)
TWO_OPERAND(A, imm8)

TWO_OPERAND_PAIR(A, T)
TWO_OPERAND_PAIR(A, reg)
TWO_OPERAND_PAIR(A, BUS)
TWO_OPERAND_PAIR(A, PSW)
TWO_OPERAND_PAIR(A, port)
TWO_OPERAND_PAIR(A, portH)
TWO_OPERAND_PAIR(A, indreg)

/** Other operands **/
TWO_OPERAND(reg, imm8)
TWO_OPERAND(reg, addr8)
TWO_OPERAND(BUS, imm8)
TWO_OPERAND(port, imm8)
TWO_OPERAND(indreg, imm8)

/******************************************************************************/
/** Instruction Decoding Tables                                              **/
/** Note: tables are here as they refer to operand functions defined above.  **/
/******************************************************************************/

optab_t base_optab[] = {

/*----------------------------------------------------------------------------
  Arithmetic
  ----------------------------------------------------------------------------*/

    MASK( "ADD", A_reg,      0xF8, 0x68, X_NONE )
    MASK( "ADD", A_indreg,   0xFE, 0x60, X_NONE )
    INSN( "ADD", A_imm8,     0x03, X_NONE )
    
    MASK( "ADDC", A_reg,     0xF8, 0x78, X_NONE )
    MASK( "ADDC", A_indreg,  0xFE, 0x70, X_NONE )
    INSN( "ADDC", A_imm8,    0x13, X_NONE )

    INSN( "INC", A,          0x17, X_NONE )
    MASK( "INC", reg,        0xF8, 0x18, X_NONE )
    MASK( "INC", indreg,     0xFE, 0x10, X_NONE )

    INSN( "DEC", A,          0x07, X_NONE )
    MASK( "DEC", reg,        0xF8, 0xC8, X_NONE )

    INSN( "DA", A,           0x57, X_NONE )

/*----------------------------------------------------------------------------
  Bitwise
  ----------------------------------------------------------------------------*/

    MASK( "ANL", A_reg,      0xF8, 0x58, X_NONE )
    MASK( "ANL", A_indreg,   0xFE, 0x50, X_NONE )
    INSN( "ANL", A_imm8,     0x53, X_NONE )
    INSN( "ANL", BUS_imm8,   0x98, X_NONE )
    RANGE("ANL", port_imm8,  0x99, 0x9A, X_NONE )
    MASK( "ANLD", portH_A,   0xFC, 0x9C, X_NONE )

    MASK( "ORL", A_reg,      0xF8, 0x48, X_NONE )
    MASK( "ORL", A_indreg,   0xFE, 0x40, X_NONE )
    INSN( "ORL", A_imm8,     0x43, X_NONE )
    INSN( "ORL", BUS_imm8,   0x88, X_NONE )
    RANGE("ORL", port_imm8,  0x89, 0x8A, X_NONE )
    MASK( "ORLD", portH_A,   0xFC, 0x8C, X_NONE )

    MASK( "XRL", A_reg,      0xF8, 0xD8, X_NONE )
    MASK( "XRL", A_indreg,   0xFE, 0xD0, X_NONE )
    INSN( "XRL", A_imm8,     0xD3, X_NONE )

    INSN( "CPL", A,          0x37, X_NONE )
    INSN( "CPL", C,          0xA7, X_NONE )
    
    INSN( "CLR", A,          0x27, X_NONE )
    INSN( "CLR", C,          0x97, X_NONE )

/*----------------------------------------------------------------------------
  Rotates
  ----------------------------------------------------------------------------*/

    INSN( "RR",  A,          0x77, X_NONE )
    INSN( "RRC", A,          0x67, X_NONE )
    INSN( "RL",  A,          0xE7, X_NONE )
    INSN( "RLC", A,          0xF7, X_NONE )

/*----------------------------------------------------------------------------
  Exchange and Swap
  ----------------------------------------------------------------------------*/

    MASK( "XCH", A_reg,      0xF8, 0x28, X_NONE )
    MASK( "XCH", A_indreg,   0xFE, 0x20, X_NONE )
    MASK( "XCHD", A_indreg,  0xFE, 0x30, X_NONE )

    INSN ( "SWAP", A,        0x47, X_NONE )

/*----------------------------------------------------------------------------
  Jump, Call and Return
  ----------------------------------------------------------------------------*/

    MASK( "JMP",   addr11,   0x1F, 0x04, X_JMP )
    INSN( "JMPP",  indA,     0xB3, X_NONE )

    MASK( "CALL",  addr11,   0x1F, 0x14, X_CALL )

    INSN( "RET",   none,     0x83, X_NONE )
    INSN( "RETR",  none,     0x93, X_NONE )

/*----------------------------------------------------------------------------
  Conditional Jumps
  ----------------------------------------------------------------------------*/

    MASK( "DJNZ", reg_addr8, 0xF8, 0xE8, X_JMP )

    INSN( "JTF",  addr8,     0x16, X_JMP )
    INSN( "JNT0", addr8,     0x26, X_JMP )
    INSN( "JT0",  addr8,     0x36, X_JMP )
    INSN( "JNT1", addr8,     0x46, X_JMP )
    INSN( "JT1",  addr8,     0x56, X_JMP )
    INSN( "JF1",  addr8,     0x76, X_JMP )
    INSN( "JNI",  addr8,     0x86, X_JMP )
    INSN( "JNZ",  addr8,     0x96, X_JMP )
    INSN( "JF0",  addr8,     0xB6, X_JMP )
    INSN( "JZ",   addr8,     0xC6, X_JMP )
    INSN( "JNC",  addr8,     0xE6, X_JMP )
    INSN( "JC",   addr8,     0xF6, X_JMP )

    INSN( "JB0",  addr8,     0x12, X_JMP )
    INSN( "JB1",  addr8,     0x32, X_JMP )
    INSN( "JB2",  addr8,     0x52, X_JMP )
    INSN( "JB3",  addr8,     0x72, X_JMP )
    INSN( "JB4",  addr8,     0x92, X_JMP )
    INSN( "JB5",  addr8,     0xB2, X_JMP )
    INSN( "JB6",  addr8,     0xD2, X_JMP )
    INSN( "JB7",  addr8,     0xF2, X_JMP )

/*----------------------------------------------------------------------------
  Moves, including Ins and Outs
  ----------------------------------------------------------------------------*/

    INSN( "MOV", A_imm8,     0x23, X_NONE )
    MASK( "MOV", reg_imm8,   0xF8, 0xB8, X_NONE )

    MASK( "MOV", A_indreg,   0xFE, 0xF0, X_NONE )
    MASK( "MOV", indreg_A,   0xFE, 0xA0, X_NONE )

    MASK( "MOV", indreg_imm8,0xFE, 0xB0, X_NONE )

    MASK( "MOV", A_reg,      0xF8, 0xF8, X_NONE )
    MASK( "MOV", reg_A,      0xF8, 0xA8, X_NONE )

    INSN( "MOV", A_PSW,      0xC7, X_NONE )
    INSN( "MOV", PSW_A,      0xD7, X_NONE )

    INSN( "MOV", A_T,        0x42, X_NONE )
    INSN( "MOV", T_A,        0x62, X_NONE )

    MASK( "MOVX", A_indreg,  0xFE, 0x80, X_NONE )
    MASK( "MOVX", indreg_A,  0xFE, 0x90, X_NONE )

    INSN( "MOVP", A_indA,    0xA3, X_NONE )
    INSN( "MOVP3", A_indA,   0xE3, X_NONE )

    MASK( "MOVD", A_portH,   0xFC, 0x0C, X_NONE )
    MASK( "MOVD", portH_A,   0xFC, 0x3C, X_NONE )

    /* --- */

    INSN( "OUTL", BUS_A,     0x02, X_NONE )
    RANGE( "OUTL", port_A,   0x39, 0x3A, X_NONE )

    RANGE( "IN", A_port,     0x09, 0x0A, X_NONE )
    INSN( "INS", A_BUS,      0x08, X_NONE )

/*----------------------------------------------------------------------------
  System
  ----------------------------------------------------------------------------*/

    INSN( "EN", I,           0x05, X_NONE )
    INSN( "DIS", I,          0x15, X_NONE )
    INSN( "EN", TCNTI,       0x25, X_NONE )
    INSN( "DIS", TCNTI,      0x35, X_NONE )
    INSN( "STRT", CNT,       0x45, X_NONE )
    INSN( "STRT", T,         0x55, X_NONE )
    INSN( "STOP", TCNT,      0x65, X_NONE )
    INSN( "ENT0", CLK,       0x75, X_NONE )
    INSN( "CLR", F0,         0x85, X_NONE )
    INSN( "CPL", F0,         0x95, X_NONE )
    INSN( "CLR", F1,         0xA5, X_NONE )
    INSN( "CPL", F1,         0xB5, X_NONE )
    INSN( "SEL", RB0,        0xC5, X_NONE )
    INSN( "SEL", RB1,        0xD5, X_NONE )
    INSN( "SEL", MB0,        0xE5, X_NONE )
    INSN( "SEL", MB1,        0xF5, X_NONE )

/*----------------------------------------------------------------------------
  Miscellaneous
  ----------------------------------------------------------------------------*/

    INSN( "NOP", none,       0x00, X_NONE )

/*----------------------------------------------------------------------------*/

   END
};

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
