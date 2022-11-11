/*****************************************************************************
 *
 * Copyright (C) 2022, Neil Johnson
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
 * Note: The PIC18 instruction encoding is based on 16-bit words.  Because of 
 *       this the base instruction unit is set to UWORD.
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

DASM_PROFILE( "dasmpic18", "Microchip PIC18", 4, 9, 0, 2, 1 )

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/
 
/* Common output formats */
#define FORMAT_NUM_8BIT         "$%02X"
#define FORMAT_NUM_16BIT        "$%04X"
#define FORMAT_NUM_24BIT		"$%06X"
#define FORMAT_REG              "%d"

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
 * Full-range (8-bit) register addressing.
 *    Register number is encoded in the opcode:
 *  15       8 7       0
 *   ---- ---- ffff ffff
 ************************************************************/
OPERAND_FUNC(f)
{
    BYTE reg = opc & 0x00FF;
    
    operand( FORMAT_REG, reg );
}

/***********************************************************
 * 4-bit immediate literal
 *  15       8 7       0
 *   ---- ---- ---- iiii
 ************************************************************/
OPERAND_FUNC(imm4)
{
    BYTE imm4 = opc & 0x000F;
    
    operand( FORMAT_NUM_8BIT, imm4 );
}

/***********************************************************
 * 8-bit immediate literal
 *  15       8 7       0
 *   ---- ---- iiii iiii
 ************************************************************/
OPERAND_FUNC(imm8)
{
    BYTE imm8 = opc & 0x00FF;
    
    operand( FORMAT_NUM_8BIT, imm8 );
}

/***********************************************************
 * Write destination: 0 = WREG, 1 = F
 *  15       8 7       0
 *   ---- --d- ---- ----
 ************************************************************/
OPERAND_FUNC(d)
{
	BYTE d = !!(opc & BIT(9));
	
	operand( d ? "F" : "W" );
}

/***********************************************************
 * Bank select: 0 = force Access, 1 = BSR select
 *  15       8 7       0
 *   ---- ---a ---- ----
 ************************************************************/
OPERAND_FUNC(a)
{
	BYTE a = !!(opc & BIT(8));
	
	operand( a ? "B" : "A" );
}

/***********************************************************
 * Bit select
 *  15       8 7       0
 *   ---- bbb- ---- ----
 ************************************************************/
OPERAND_FUNC(b)
{
	BYTE b = ( opc >> 9 ) & 0x07;
	
	operand( "%d", b );
}

/***********************************************************
 * Select flag in bit 0
 *  15       8 7       0
 *   ---- ---- ---- ---s
 ************************************************************/
OPERAND_FUNC(s0)
{
	BYTE s0 = opc & BIT(0);
	
	operand( "%d", !!s0 );
}

/***********************************************************
 * Relative address
 *  15       8 7       0
 *   ---- ---- rrrr rrrr
 ************************************************************/
OPERAND_FUNC(rel8)
{
    BYTE disp = opc & 0xFF;
    ADDR dest = *addr + disp;
    
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

/***********************************************************
 * FSR register number
 *  15       8 7       0
 *   ---- ---- --rr ----
 ************************************************************/
OPERAND_FUNC(fsr)
{
	BYTE fsr = ( opc >> 4 ) & 0x03;
	
	operand( "%d", fsr);
}

/***********************************************************
 * Source and destination addresses in a 32-bit opcode
 *  15       8 7       0
 *   ---- ssss ssss ssss
 *   ---- dddd dddd dddd
 ************************************************************/
OPERAND_FUNC(fs_fd)
{
	ADDR addr12 = opc & 0x0FFF;
	operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr12 ) );
    xref_addxref( xtype, g_insn_addr, addr12 );
    
	COMMA;

	opc = nextw( f, addr );
	addr12 = opc & 0x0FFF;
	operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr12 ) );
    xref_addxref( xtype, g_insn_addr, addr12 );
}

/***********************************************************
 * 20-bit address
 *  15       8 7       0
 *   ---- ---- aaaa aaaa  <- a[7:0]
 *   ---- aaaa aaaa aaaa  <- a[19:8]
 ************************************************************/
OPERAND_FUNC(addr20)
{
	ADDR lo = opc & 0x00FF;
	ADDR hi = nextw( f, addr );
	hi &= 0x0FFF;
	hi <<= 8;
	hi |= lo;
	
	operand( xref_genwordaddr( NULL, FORMAT_NUM_24BIT, hi ) );
    xref_addxref( xtype, g_insn_addr, hi );
}

/***********************************************************
 * 20-bit address with s bit in position 8
 *  15       8 7       0
 *   ---- ---s aaaa aaaa  <- a[7:0]
 *   ---- aaaa aaaa aaaa  <- a[19:8]
 ************************************************************/
OPERAND_FUNC(addr20_s8)
{
	ADDR lo = opc & 0x00FF;
	BYTE s8 = !!(opc & BIT(8));
	ADDR hi = nextw( f, addr );
	hi &= 0x0FFF;
	hi <<= 8;
	hi |= lo;
	
	operand( xref_genwordaddr( NULL, FORMAT_NUM_24BIT, hi ) );
    xref_addxref( xtype, g_insn_addr, hi );
    COMMA;
    operand( "%d", !!s8 );
}

/***********************************************************
 * 11-bit Relative address
 *  15       8 7       0
 *   ---- -rrr rrrr rrrr
 ************************************************************/
OPERAND_FUNC(rel11)
{
    WORD disp = opc & 0x3FF;
    ADDR dest = *addr + disp;
    
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

/***********************************************************
 * 12-bit immediate
 *  15       8 7       0
 *   ---- ---- ---- nnnn
 *   ---- ---- nnnn nnnn
 ************************************************************/
OPERAND_FUNC(imm12)
{
	UWORD hi = opc & 0x000F;
	UWORD lo = nextw( f, addr );
	lo &= 0x00FF;
	hi <<= 8;
	hi |= lo;
	
	operand( FORMAT_NUM_24BIT, hi );
}

/******************************************************************************/
/**                          Multiple Operands                               **/
/******************************************************************************/

TWO_OPERAND(f, a)
TWO_OPERAND(fsr, imm12)

THREE_OPERAND(f, b, a)
THREE_OPERAND(f, d, a)

/******************************************************************************/
/** Instruction Decoding Tables                                              **/
/** Note: tables are here as they refer to operand functions defined above.  **/
/******************************************************************************/

optab_t base_optab[] = {
    
    /* Byte-Oriented Operations */
    MASK ( "ADDWF",  f_d_a,         0xFC00, 0x2400, X_NONE )
    MASK ( "ADDWFC", f_d_a,         0xFC00, 0x2000, X_NONE )
    MASK ( "ANDWF",  f_d_a,         0xFC00, 0x1400, X_NONE )
    MASK ( "CLRF",   f_a,           0xFE00, 0x6A00, X_NONE )    
    MASK ( "COMF",   f_d_a,         0xFC00, 0x1C00, X_NONE )
    MASK ( "CPFSEQ", f_a,           0xFE00, 0x6200, X_NONE )
    MASK ( "CPFSGT", f_a,           0xFE00, 0x6400, X_NONE )
    MASK ( "CPFSLT", f_a,           0xFE00, 0x6000, X_NONE )
    MASK ( "DECF",   f_d_a,         0xFC00, 0x0400, X_NONE )
    MASK ( "DECFSZ", f_d_a,         0xFC00, 0x2C00, X_NONE )
    MASK ( "DCFSNZ", f_d_a,         0xFC00, 0x4C00, X_NONE )
    MASK ( "INCF",   f_d_a,         0xFC00, 0x2800, X_NONE )
    MASK ( "INCFSZ", f_d_a,         0xFC00, 0x3C00, X_NONE )
    MASK ( "INFSNZ", f_d_a,         0xFC00, 0x4800, X_NONE )
    MASK ( "IORWF",  f_d_a,         0xFC00, 0x1000, X_NONE )
    MASK ( "MOVF",   f_d_a,         0xFC00, 0x5000, X_NONE )
    MASK ( "MOVFF",  fs_fd,         0xF000, 0xC000, X_NONE )
    MASK ( "MOVWF",  f_a,           0xFC00, 0x6C00, X_NONE )
    MASK ( "MULWF",  f_a,           0xFC00, 0x0200, X_NONE )
    MASK ( "NEGF",   f_a,           0xFC00, 0x6C00, X_NONE )
    MASK ( "RLCF",   f_d_a,         0xFC00, 0x3400, X_NONE )
    MASK ( "RLNCF",  f_d_a,         0xFC00, 0x4400, X_NONE )
    MASK ( "RRCF",   f_d_a,         0xFC00, 0x3000, X_NONE )
    MASK ( "RRNCF",  f_d_a,         0xFC00, 0x4000, X_NONE )
    MASK ( "SETF",   f_a,           0xFE00, 0x6800, X_NONE )
    MASK ( "SUBFWB", f_d_a,         0xFC00, 0x5400, X_NONE )
    MASK ( "SUBFW",  f_d_a,         0xFC00, 0x5C00, X_NONE )
    MASK ( "SUBWFB", f_d_a,         0xFC00, 0x5800, X_NONE )
    MASK ( "SWAPF",  f_d_a,         0xFC00, 0x3800, X_NONE )
    MASK ( "TSTFSZ", f_a,           0xFE00, 0x6600, X_NONE )
    MASK ( "XORWF",  f_d_a,         0xFC00, 0x1800, X_NONE )  
        
    /* Bit-Oriented Operations */
    
    MASK ( "BCF",    f_b_a,         0xF000, 0x9000, X_NONE )
    MASK ( "BSF",    f_b_a,         0xF000, 0x8000, X_NONE )
    MASK ( "BTFSC",  f_b_a,         0xF000, 0xB000, X_NONE )
    MASK ( "BTFSS",  f_b_a,         0xF000, 0xA000, X_NONE )
    MASK ( "BTG",    f_b_a,         0xF000, 0x7000, X_NONE )
        
    /* Control Operations */
    
    MASK ( "BC",     rel8,          0xFF00, 0xE200, X_NONE )
    MASK ( "BN",     rel8,          0xFF00, 0xE600, X_NONE )
    MASK ( "BNC",    rel8,          0xFF00, 0xE300, X_NONE )
    MASK ( "BNN",    rel8,          0xFF00, 0xE700, X_NONE )
    MASK ( "BNOV",   rel8,          0xFF00, 0xE500, X_NONE )
    MASK ( "BNZ",    rel8,          0xFF00, 0xE100, X_NONE )
    MASK ( "BOV",    rel8,          0xFF00, 0xE400, X_NONE )
    MASK ( "BZ",     rel8,          0xFF00, 0xE000, X_NONE )
    MASK ( "BRA",    rel8,          0xF800, 0xD000, X_NONE )
    
    MASK ( "CALL",   addr20_s8,     0xFE00, 0xEC00, X_NONE )
    INSN ( "CLRWDT", none,          0x0004,         X_NONE )
    INSN ( "DAW",    none,          0x0007,         X_NONE )
    MASK ( "GOTO",   addr20,        0xFF00, 0xEF00, X_NONE )
    
    INSN ( "NOP",    none,          0x0000,         X_NONE )
    MASK ( "NOP",    none,          0xF000, 0xF000, X_NONE )
    
    INSN ( "POP",    none,          0x0006,         X_NONE )
    INSN ( "PUSH",   none,          0x0005,         X_NONE )
    MASK ( "RCALL",  rel11,         0xF800, 0xD800, X_NONE )
    INSN ( "RESET",  none,          0x00FF,         X_NONE )
    
    MASK ( "RETFIE", s0,            0xFFFE, 0x0010, X_NONE )
    MASK ( "RETLW",  imm8,          0xFF00, 0x0C00, X_NONE )
    MASK ( "RETURN", s0,            0xFFFE, 0x0012, X_NONE )
     
    INSN ( "SLEEP",  none,          0x0003,         X_NONE )
    
    /* Literal Operations */
        
    MASK ( "ADDLW",  imm8,          0xFF00, 0x0F00, X_NONE )
    MASK ( "ANDLW",  imm8,          0xFF00, 0x0B00, X_NONE )
    MASK ( "IORLW",  imm8,          0xFF00, 0x0900, X_NONE )
    MASK ( "LFSR",   fsr_imm12,     0xFFC0, 0xEE00, X_NONE )
    MASK ( "MOVLB",  imm4,          0xFFF0, 0x0100, X_NONE )
    MASK ( "MOVLW",  imm8,          0xFF00, 0x0E00, X_NONE )
    MASK ( "MOVLW",  imm8,          0xFF00, 0x0E00, X_NONE )
    MASK ( "RETLW",  imm8,          0xFF00, 0x0C00, X_NONE )
    MASK ( "SUBLW",  imm8,          0xFF00, 0x0800, X_NONE )
    MASK ( "XORLW",  imm8,          0xFF00, 0x0A00, X_NONE )
    
    /* Data Memory <-> Program Memory Operations */
    
    INSN ( "TBLRD*",  none,         0x0008,         X_NONE )
    INSN ( "TBLRD*+", none,         0x0009,         X_NONE )
    INSN ( "TBLRD*-", none,         0x000A,         X_NONE )
    INSN ( "TBLRD+*", none,         0x000B,         X_NONE )
    
    INSN ( "TBLWT*",  none,         0x000C,         X_NONE )
    INSN ( "TBLWT*+", none,         0x000D,         X_NONE )
    INSN ( "TBLWT*-", none,         0x000E,         X_NONE )
    INSN ( "TBLWT+*", none,         0x000F,         X_NONE )

/*----------------------------------------------------------------------------*/  
    
    END
};

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
