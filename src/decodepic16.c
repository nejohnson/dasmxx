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
 * Note: The PIC16 instruction encoding is based on 14-bit words.  Because of 
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

DASM_PROFILE( "dasmpic16", "Microchip PIC16", 4, 9, 0, 2, 1 )

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
 * Full-range (7-bit) register addressing.
 *    Register number is encoded in the opcode:
 *  15       8 7       0
 *   ---- ---- -fff ffff
 ************************************************************/
OPERAND_FUNC(f)
{
    BYTE reg = opc & 0x003F;
    
    operand( FORMAT_REG, reg );
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
 * 11-bit address fragment
 *  15       8 7       0
 *   ---- -aaa aaaa aaaa
 ************************************************************/
OPERAND_FUNC(addr11)
{
    UWORD addr11 = opc & 0x03FF;
    
    operand( "%s", xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr11 ) );
    xref_addxref( xtype, g_insn_addr, addr11 );
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
    int d = ( ( opc >> 7 ) & 0x0001 );
    
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
    int b = ( ( opc >> 7 ) & 0x0007 );
    
    operand_f( f, addr, opc, xtype );
    COMMA;
    operand( FORMAT_REG, b );
}

/******************************************************************************/
/** Instruction Decoding Tables                                              **/
/** Note: tables are here as they refer to operand functions defined above.  **/
/******************************************************************************/

optab_t base_optab[] = {

    MASK ( "NOP",    none,          0x3F9F, 0x0000, X_NONE )
    
    /* Byte-Oriented Operations */
    MASK ( "ADDWF",  f_d,           0x3F00, 0x0300, X_NONE )
    MASK ( "ANDWF",  f_d,           0x3F00, 0x0500, X_NONE )
    MASK ( "COMF",   f_d,           0x3F00, 0x0900, X_NONE )
    MASK ( "DECF",   f_d,           0x3F00, 0x0300, X_NONE )
    MASK ( "DECFSZ", f_d,           0x3F00, 0x0B00, X_NONE )
    MASK ( "INCF",   f_d,           0x3F00, 0x0A00, X_NONE )
    MASK ( "INCFSZ", f_d,           0x3F00, 0x0F00, X_NONE )
    MASK ( "IORWF",  f_d,           0x3F00, 0x0400, X_NONE )
    MASK ( "MOVF",   f_d,           0x3F00, 0x0800, X_NONE )    
    MASK ( "RLF",    f_d,           0x3F00, 0x0D00, X_NONE )
    MASK ( "RRF",    f_d,           0x3F00, 0x0C00, X_NONE )
    MASK ( "SUBWF",  f_d,           0x3F00, 0x0200, X_NONE )
    MASK ( "SWAPF",  f_d,           0x3F00, 0x0E00, X_NONE )    
    MASK ( "XORWF",  f_d,           0x3F00, 0x0600, X_NONE )
    
    MASK ( "CLRF",   f,             0x3F80, 0x0180, X_NONE )    
    MASK ( "MOVWF",  f,             0x3F80, 0x0080, X_NONE )
    
    MASK ( "CLRW",   none,          0x3F80, 0x0100, X_NONE )    
        
    /* Bit-Oriented Operations */
    
    MASK ( "BCF",    f_b,           0x3C00, 0x1000, X_NONE )
    MASK ( "BSF",    f_b,           0x3C00, 0x1400, X_NONE )
    MASK ( "BTFSC",  f_b,           0x3C00, 0x1800, X_NONE )
    MASK ( "BTFSS",  f_b,           0x3C00, 0x1C00, X_NONE )
        
    /* Literal and Control Operations */
        
    MASK ( "ADDLW",  imm8,          0x3E00, 0x3E00, X_NONE )
    MASK ( "ANDLW",  imm8,          0x3F00, 0x3900, X_NONE )
    MASK ( "CALL",   addr11,        0x3800, 0x2000, X_NONE )
    MASK ( "GOTO",   addr11,        0x3800, 0x2800, X_NONE )
    MASK ( "IORLW",  imm8,          0x3F00, 0x3800, X_NONE )
    MASK ( "MOVLW",  imm8,          0x3C00, 0x3000, X_NONE )
    MASK ( "RETLW",  imm8,          0x3C00, 0x3400, X_NONE )
    MASK ( "SUBLW",  imm8,          0x3E00, 0x3C00, X_NONE )
    MASK ( "XORLW",  imm8,          0x3F00, 0x3A00, X_NONE )
    
    INSN ( "CLRWDT", none,          0x0064,         X_NONE )
    INSN ( "OPTION", none,          0x0002,         X_NONE )
    INSN ( "SLEEP",  none,          0x0063,         X_NONE )
    INSN ( "RETFIE", none,          0x0009,         X_NONE )
    INSN ( "RETURN", none,          0x0008,         X_NONE )

/*----------------------------------------------------------------------------*/  
    
    END
};

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
