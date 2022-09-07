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
 * Note: The PIC instruction encoding is based on 12-bit words.  Because of 
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

DASM_PROFILE( "dasmpic12", "Microchip PIC10/PIC12", 4, 9, 0, 2 )

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/
 
/* Common output formats */
#define FORMAT_NUM_8BIT         "$%02X"
#define FORMAT_NUM_16BIT        "$%04X"
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
 * Full-range (5-bit) register addressing.
 *    Register number is encoded in the opcode:
 *  15       8 7       0
 *   ---- ---- ---f ffff
 ************************************************************/
OPERAND_FUNC(f)
{
    BYTE reg = opc & 0x001F;
    
    operand( FORMAT_REG, reg );
}

/***********************************************************
 * Reduced-range (3-bit) register addressing.
 *    Register number is encoded in the opcode:
 *  15       8 7       0
 *   ---- ---- ---- -fff
 ************************************************************/
OPERAND_FUNC(f3)
{
    BYTE f3 = opc & 0x0007;
    
    operand( FORMAT_REG, f3 );
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
 * 8-bit address fragment
 *  15       8 7       0
 *   ---- ---- aaaa aaaa
 * 
 * However, the actual address is composed of three parts:
 *    PC[7:0]  <= addr8
 *    PC[8]    <= 0
 *    PC[10:9] <= STATUS_REG[6:5]
 * 
 * I guess the idea is you treat the two bits in the STATUS
 * register as a code bank selector and you can jump into the
 * bottom 256 addresses.
 ************************************************************/
OPERAND_FUNC(addr8)
{
    BYTE addr8 = opc & 0x00FF;
    
    operand( FORMAT_NUM_8BIT, addr8 );
}

/***********************************************************
 * 9-bit address fragment
 *  15       8 7       0
 *   ---- ---a aaaa aaaa
 * 
 * However, the actual address is composed of three parts:
 *    PC[8:0]  <= addr9
 *    PC[10:9] <= STATUS_REG[6:5]
 * 
 * I guess the idea is you treat the two bits in the STATUS
 * register as a code bank selector.
 ************************************************************/
OPERAND_FUNC(addr9)
{
    UWORD addr9 = opc & 0x01FF;
    
    operand( FORMAT_NUM_16BIT, addr9 );
}

/******************************************************************************/
/**                            Double Operands                               **/
/******************************************************************************/

/***********************************************************
 * Register and destination
 *  15       8 7       0
 *   ---- ---- --df ffff
 ************************************************************/
OPERAND_FUNC(f_d)
{
    int d = ( ( opc >> 5 ) & 0x0001 );
    
    operand_f( f, addr, opc, xtype );
    COMMA;
    operand( FORMAT_REG, d );
}

/***********************************************************
 * Register and bit index
 *  15       8 7       0
 *   ---- ---- bbbf ffff
 ************************************************************/
OPERAND_FUNC(f_b)
{
    int b = ( ( opc >> 5 ) & 0x0007 );
    
    operand_f( f, addr, opc, xtype );
    COMMA;
    operand( FORMAT_REG, b );
}

/******************************************************************************/
/** Instruction Decoding Tables                                              **/
/** Note: tables are here as they refer to operand functions defined above.  **/
/******************************************************************************/

optab_t base_optab[] = {

    INSN ( "NOP",    none,          0x0000,         X_NONE )
    
    /* Byte-Oriented Operations */
    MASK ( "ADDWF",  f_d,           0x0FC0, 0x01C0, X_NONE )
    MASK ( "ANDWF",  f_d,           0x0FC0, 0x0140, X_NONE )
    MASK ( "COMF",   f_d,           0x0FC0, 0x0240, X_NONE )
    MASK ( "DECF",   f_d,           0x0FC0, 0x00C0, X_NONE )
    MASK ( "DECFSZ", f_d,           0x0FC0, 0x02C0, X_NONE )
    MASK ( "INCF",   f_d,           0x0FC0, 0x0280, X_NONE )
    MASK ( "INCFSZ", f_d,           0x0FC0, 0x03C0, X_NONE )
    MASK ( "IORWF",  f_d,           0x0FC0, 0x0100, X_NONE )    
    MASK ( "RLF",    f_d,           0x0FC0, 0x0340, X_NONE )
    MASK ( "RRF",    f_d,           0x0FC0, 0x0300, X_NONE )
    MASK ( "SUBWF",  f_d,           0x0FC0, 0x0080, X_NONE )
    MASK ( "SWAPF",  f_d,           0x0FC0, 0x0380, X_NONE )    
    MASK ( "XORWF",  f_d,           0x0FC0, 0x0180, X_NONE )
    
    MASK ( "CLRF",   f,             0x0FE0, 0x0060, X_NONE )    
    MASK ( "MOVWF",  f,             0x0FE0, 0x0020, X_NONE )
    
    INSN ( "CLRW",   none,          0x0040,         X_NONE )    
        
    /* Bit-Oriented Operations */
    
    MASK ( "BCF",    f_b,           0x0F00, 0x0400, X_NONE )
    MASK ( "BSF",    f_b,           0x0F00, 0x0500, X_NONE )
    MASK ( "BTFSC",  f_b,           0x0F00, 0x0600, X_NONE )
    MASK ( "BTFSS",  f_b,           0x0F00, 0x0700, X_NONE )
        
    /* Literal and Control Operations */
        
    MASK ( "ANDLW",  imm8,          0x0F00, 0x0E00, X_NONE )
    MASK ( "CALL",   addr8,         0x0F00, 0x0900, X_NONE )
    MASK ( "GOTO",   addr9,         0x0E00, 0x0A00, X_NONE )
    MASK ( "IORLW",  imm8,          0x0F00, 0x0D00, X_NONE )
    MASK ( "MOVLW",  imm8,          0x0F00, 0x0C00, X_NONE )
    MASK ( "RETLW",  imm8,          0x0F00, 0x0800, X_NONE )
    MASK ( "XORLW",  imm8,          0x0F00, 0x0F00, X_NONE )
    
    MASK ( "TRIS",   f3,            0x0FF8, 0x0000, X_NONE )
    INSN ( "CLRWDT", none,          0x0004,         X_NONE )
    INSN ( "OPTION", none,          0x0002,         X_NONE )
    INSN ( "SLEEP",  none,          0x0003,         X_NONE )

/*----------------------------------------------------------------------------*/  
    
    END
};

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
