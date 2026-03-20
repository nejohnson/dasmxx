/*****************************************************************************
 *
 * Copyright (C) 2014-2026, Neil Johnson
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

DASM_PROFILE( "dasmm8", "ST Micro STM8", 3, 9, 0, 1, 1 )

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/

/* Common output formats */
#define FORMAT_NUM_8BIT         "$%02X"
#define FORMAT_NUM_16BIT        "$%04X"
#define FORMAT_NUM_24BIT        "$%06X"

/* Construct a 16-bit word out of low and high bytes */
#define MK_WORD(l,h)            ( ((l) & 0xFF) | (((h) & 0xFF) << 8) )

/* Construct a 24-bit word out of low, mid and high bytes */
#define MK_LONG_WORD(l,m,h)     ( MK_WORD(l,m) | (((h) & 0xFF) << 16) )

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
/**                            Registers                                     **/
/******************************************************************************/

OPERAND_FUNC(A)
{
    operand( "A" );
}

OPERAND_FUNC(CC)
{
    operand( "CC" );
}

OPERAND_FUNC(X)
{
    operand( "X" );
}

OPERAND_FUNC(indX)
{
    operand( "(X)" );
}

OPERAND_FUNC(XL)
{
    operand( "XL" );
}

OPERAND_FUNC(Y)
{
    operand( "Y" );
}

OPERAND_FUNC(indY)
{
    operand( "(Y)" );
}

OPERAND_FUNC(YL)
{
    operand( "YL" );
}

OPERAND_FUNC(SP)
{
    operand( "SP" );
}

/******************************************************************************/
/**                            Single Operands                               **/
/******************************************************************************/

/***********************************************************
 * Process immediate 8-bit operands.
 ************************************************************/

OPERAND_FUNC(imm8)
{
    UBYTE byte = next( f, addr );

    operand( "#" FORMAT_NUM_8BIT, byte );
}

/***********************************************************
 * Process short 8-bit offset operands.
 ************************************************************/

OPERAND_FUNC(off8)
{
    UBYTE byte = next( f, addr );

    operand( FORMAT_NUM_8BIT, byte );
}

/***********************************************************
 * Process immediate 16-bit operands.
 ************************************************************/

OPERAND_FUNC(imm16)
{
    UWORD word = nextw( f, addr );

    operand( "#" FORMAT_NUM_16BIT, word );
}

OPERAND_FUNC(off16)
{
    UWORD word = nextw( f, addr );

    operand( FORMAT_NUM_16BIT, word );
}

/***********************************************************
 * Process short absolute address (8-bit) operands.
 ************************************************************/

OPERAND_FUNC(mem8)
{
    ADDR addr8 = (ADDR)next( f, addr );

    operand( xref_genwordaddr( NULL, FORMAT_NUM_8BIT, addr8 ) );
    xref_addxref( xtype, g_insn_addr, addr8 );
}

OPERAND_FUNC(ind8)
{
    operand( "[" );
    operand_mem8( f, addr, opc, xtype );
    operand( "]" );
}

OPERAND_FUNC(rel8)
{
    BYTE disp = (BYTE)next( f, addr );
    ADDR dest = *addr + disp;

    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

/***********************************************************
 * Process long absolute address (16-bit) operand.
 ************************************************************/

OPERAND_FUNC(mem16)
{
    UBYTE low_addr  = next( f, addr );
    UBYTE high_addr = next( f, addr );
    ADDR addr16     = MK_WORD( low_addr, high_addr );

    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr16 ) );
    xref_addxref( xtype, g_insn_addr, addr16 );
}

OPERAND_FUNC(ind16)
{
    operand( "[" );
    operand_mem16( f, addr, opc, xtype );
    operand( "]" );
}

/***********************************************************
 * Process extended absolute address (24-bit) operand.
 ************************************************************/

OPERAND_FUNC(mem24)
{
    UBYTE low_addr  = next( f, addr );
    UBYTE mid_addr  = next( f, addr );
    UBYTE high_addr = next( f, addr );
    ADDR addr24     = MK_LONG_WORD( low_addr, mid_addr, high_addr );

    operand( xref_genwordaddr( NULL, FORMAT_NUM_24BIT, addr24 ) );
    xref_addxref( xtype, g_insn_addr, addr24 );
}

/******************************************************************************/
/**                            Double Operands                               **/
/******************************************************************************/

TWO_OPERAND(X, Y)
TWO_OPERAND(X, A)
TWO_OPERAND(Y, A)

TWO_OPERAND(A, imm8)
TWO_OPERAND(A, ind8)
TWO_OPERAND(A, ind16)
TWO_OPERAND(A, mem8)
TWO_OPERAND(A, mem16)
TWO_OPERAND(A, indX)

TWO_OPERAND(X,  imm16)
TWO_OPERAND(X,  mem16)
TWO_OPERAND(Y,  imm16)
TWO_OPERAND(Y,  mem16)
TWO_OPERAND(SP, imm8)

OPERAND_FUNC(off8_SP)
{
    operand( "(" );
    operand_off8( f, addr, opc, xtype );
    COMMA;
    operand_SP( f, addr, opc, xtype );
    operand( ")" );
}

OPERAND_FUNC(off8_X)
{
    operand( "(" );
    operand_off8( f, addr, opc, xtype );
    COMMA;
    operand_X( f, addr, opc, xtype );
    operand( ")" );
}

OPERAND_FUNC(off8_Y)
{
    operand( "(" );
    operand_off8( f, addr, opc, xtype );
    COMMA;
    operand_Y( f, addr, opc, xtype );
    operand( ")" );
}

OPERAND_FUNC(off16_X)
{
    operand( "(" );
    operand_off16( f, addr, opc, xtype );
    COMMA;
    operand_X( f, addr, opc, xtype );
    operand( ")" );
}

OPERAND_FUNC(off16_Y)
{
    operand( "(" );
    operand_off16( f, addr, opc, xtype );
    COMMA;
    operand_Y( f, addr, opc, xtype );
    operand( ")" );
}

OPERAND_FUNC(ind8_X)
{
    operand( "(" );
    operand_ind8( f, addr, opc, xtype );
    COMMA;
    operand_X( f, addr, opc, xtype );
    operand( ")" );
}

OPERAND_FUNC(ind8_Y)
{
    operand( "(" );
    operand_ind8( f, addr, opc, xtype );
    COMMA;
    operand_Y( f, addr, opc, xtype );
    operand( ")" );
}

OPERAND_FUNC(ind16_X)
{
    operand( "(" );
    operand_ind16( f, addr, opc, xtype );
    COMMA;
    operand_X( f, addr, opc, xtype );
    operand( ")" );
}

OPERAND_FUNC(ind16_Y)
{
    operand( "(" );
    operand_ind16( f, addr, opc, xtype );
    COMMA;
    operand_Y( f, addr, opc, xtype );
    operand( ")" );
}

/* The STM8 does not store instruction operands in order so we need to
 * extract them first and then emit them in the correct order.
 */
OPERAND_FUNC(mem16_bit)
{
    UBYTE pos = next( f, addr );

    operand_mem16( f, addr, opc, xtype );
    COMMA;
    operand( "#%d", (pos >> 1) & 0x07 );
}

OPERAND_FUNC(mem16_imm8)
{
    UBYTE byte = next( f, addr );

    operand_mem16( f, addr, opc, xtype );
    COMMA;
    operand( "#" FORMAT_NUM_8BIT, byte );
}

OPERAND_FUNC(mem8_mem8)
{
    ADDR src = (ADDR)next( f, addr );

    operand_mem8( f, addr, opc, xtype );
    COMMA;
    operand( xref_genwordaddr( NULL, FORMAT_NUM_8BIT, src ) );
    xref_addxref( xtype, g_insn_addr, src );
}

OPERAND_FUNC(mem16_mem16)
{
    ADDR src = (ADDR)nextw( f, addr );

    operand_mem16( f, addr, opc, xtype );
    COMMA;
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, src ) );
    xref_addxref( xtype, g_insn_addr, src );
}


/******************************************************************************/
/**                      Composite Double Operands                           **/
/******************************************************************************/

TWO_OPERAND(A, ind8_X)
TWO_OPERAND(A, ind8_Y)
TWO_OPERAND(A, off8_X)
TWO_OPERAND(A, off8_Y)
TWO_OPERAND(A, off8_SP)

TWO_OPERAND(A, ind16_X)
TWO_OPERAND(A, off16_X)
TWO_OPERAND(A, off16_Y)

/******************************************************************************/
/**                      Composite Triple Operands                           **/
/******************************************************************************/

TWO_OPERAND(mem16_bit, rel8)

/******************************************************************************/
/** Instruction Decoding Tables                                              **/
/** Note: tables are here as they refer to operand functions defined above.  **/
/******************************************************************************/

#define ALU_OPS \
    ALU_OP( "adc", 0x09 ) \
    ALU_OP( "add", 0x0B ) \
    ALU_OP( "and", 0x04 ) \
    ALU_OP( "bcp", 0x05 ) \
    ALU_OP( "cp" , 0x01 ) \
    ALU_OP( "or" , 0x0A ) \
    ALU_OP( "sbc", 0x02 ) \
    ALU_OP( "sub", 0x00 ) \
    ALU_OP( "xor", 0x08 )

#define BYTE_OPS \
    BYTE_OP( "cpl", 0x03 ) \
    BYTE_OP( "dec", 0x0A ) \
    BYTE_OP( "inc", 0x0C ) \
    BYTE_OP( "neg", 0x00 ) \
    BYTE_OP( "rlc", 0x09 ) \
    BYTE_OP( "rrc", 0x06 ) \
    BYTE_OP( "sll", 0x08 ) \
    BYTE_OP( "sra", 0x07 ) \
    BYTE_OP( "srl", 0x04 ) \
    BYTE_OP( "swap",0x0E ) \
    BYTE_OP( "tnz", 0x0D )

/******************************************************************************/
/**   Table x72 - long pointer                                               **/
/******************************************************************************/

optab_t x72_optab[] = {

#undef ALU_OP
#define ALU_OP(n,b) \
    INSN ( n, A_ind16,    0xC0 | b, X_NONE ) \
    INSN ( n, A_ind16_X,  0xD0 | b, X_NONE )

    ALU_OPS

#undef BYTE_OP
#define BYTE_OP(n,b) \
    INSN ( n, mem16,   0x50 | b, X_NONE ) \
    INSN ( n, off16_X, 0x40 | b, X_NONE ) \
    INSN ( n, ind16,   0x30 | b, X_NONE ) \
    INSN ( n, ind16_X, 0x60 | b, X_NONE )

    BYTE_OPS

    INSN ( "call",  ind16,   0xCD, X_CALL )
    INSN ( "call",  ind16_X, 0xDD, X_CALL )

    INSN ( "jp",    ind16,   0xCC, X_JMP  )
    INSN ( "jp",    ind16_X, 0xDC, X_JMP  )

    INSN ( "wfe", none,       0x8F, X_NONE )

    MASK ( "btjt", mem16_bit_rel8, 0xF1, 0x00, X_JMP )
    MASK ( "btjf", mem16_bit_rel8, 0xF1, 0x01, X_JMP )

    MASK ( "bset", mem16_bit, 0xF1, 0x10, X_NONE )
    MASK ( "bres", mem16_bit, 0xF1, 0x11, X_NONE )

    END
};

/******************************************************************************/
/**   Table x90 - Y reg                                                      **/
/******************************************************************************/

optab_t x90_optab[] = {

#undef ALU_OP
#define ALU_OP(n,b) \
    INSN ( n, A_ind8_Y,   0xF0 | b, X_NONE ) \
    INSN ( n, A_off8_Y,   0xE0 | b, X_NONE ) \
    INSN ( n, A_off16_Y,  0xD0 | b, X_NONE )

    ALU_OPS

#undef BYTE_OP
#define BYTE_OP(n,b) \
    INSN ( n, ind8_Y,     0x70 | b, X_NONE ) \
    INSN ( n, off8_Y,     0x60 | b, X_NONE ) \
    INSN ( n, off16_Y,    0x40 | b, X_NONE )

    BYTE_OPS

    INSN ( "negw",  Y,        0x50, X_NONE )
    INSN ( "mul", Y_A,        0x42, X_NONE )
    INSN ( "div", Y_A,        0x62, X_NONE )

    INSN ( "cplw",  Y,        0x53, X_NONE )
    INSN ( "decw",  Y,        0x5A, X_NONE )
    INSN ( "incw",  Y,        0x5C, X_NONE )

    INSN ( "clrw",  Y,        0x5F, X_NONE )

    INSN ( "rrwa",  Y,        0x01, X_NONE )
    INSN ( "rlwa",  Y,        0x02, X_NONE )
    INSN ( "rrcw",  Y,        0x56, X_NONE )
    INSN ( "rlcw",  Y,        0x59, X_NONE )

    INSN ( "srlw",  Y,        0x54, X_NONE )
    INSN ( "sraw",  Y,        0x57, X_NONE )
    INSN ( "sllw",  Y,        0x58, X_NONE )

    INSN ( "swapw", Y,        0x5E, X_NONE )
    INSN ( "tnzw",  Y,        0x5D, X_NONE )

    INSN ( "popw",  Y,        0x85, X_NONE )
    INSN ( "pushw", Y,        0x89, X_NONE )

    INSN ( "call",  indY,     0xFD, X_CALL )
    INSN ( "call",  off8_Y,   0xED, X_CALL )
    INSN ( "call",  off16_Y,  0xDD, X_CALL )

    INSN ( "jp",    indY,     0xFC, X_JMP  )
    INSN ( "jp",    off8_Y,   0xEC, X_JMP  )
    INSN ( "jp",    off16_Y,  0xDC, X_JMP  )

    MASK ( "bcpl", mem16_bit, 0xF1, 0x10, X_NONE )

    INSN ( "jrnh",  rel8,     0x28, X_JMP  )
    INSN ( "jrh",   rel8,     0x29, X_JMP  )
    INSN ( "jrnm",  rel8,     0x2C, X_JMP  )
    INSN ( "jrm",   rel8,     0x2D, X_JMP  )
    INSN ( "jril",  rel8,     0x2E, X_JMP  )
    INSN ( "jrih",  rel8,     0x2F, X_JMP  )

    END
};

/******************************************************************************/
/**   Table x91 - short pointer + Y                                          **/
/******************************************************************************/

optab_t x91_optab[] = {

#undef ALU_OP
#define ALU_OP(n,b) \
    INSN ( n, A_ind8_Y,    0xD0 | b, X_NONE )

    ALU_OPS

#undef BYTE_OP
#define BYTE_OP(n,b) \
    INSN ( n, ind8_Y,       0x70 | b, X_NONE )

    BYTE_OPS

    INSN ( "call",  ind8_Y,  0xDD, X_CALL )
    INSN ( "jp",    ind8_Y,  0xDC, X_JMP  )

    END
};

/******************************************************************************/
/**   Table x92 - short pointer, short pointer + X                           **/
/******************************************************************************/

optab_t x92_optab[] = {

#undef ALU_OP
#define ALU_OP(n,b) \
    INSN ( n, A_ind8,    0xC0 | b, X_NONE ) \
    INSN ( n, A_ind8_X,  0xD0 | b, X_NONE )

    ALU_OPS

#undef BYTE_OP
#define BYTE_OP(n,b) \
    INSN ( n, ind8,       0x30 | b, X_NONE ) \
    INSN ( n, ind8_X,     0x60 | b, X_NONE )

    BYTE_OPS

    INSN ( "callf", ind16,   0x8D, X_CALL )
    INSN ( "call",  ind8,    0xCD, X_CALL )
    INSN ( "call",  ind8_X,  0xDD, X_CALL )

    INSN ( "jpf",   ind16,   0xAC, X_JMP  )
    INSN ( "jp",    ind8,    0xCC, X_JMP  )
    INSN ( "jp",    ind8_X,  0xDC, X_JMP  )

    END
};

/******************************************************************************/
/**   Base Table                                                             **/
/******************************************************************************/

optab_t base_optab[] = {

/*----------------------------------------------------------------------------
  Sub Tables
  ----------------------------------------------------------------------------*/

    TABLE ( x72_optab, 0x72 )
    TABLE ( x90_optab, 0x90 )
    TABLE ( x91_optab, 0x91 )
    TABLE ( x92_optab, 0x92 )

/*----------------------------------------------------------------------------
  Load/Store
  ----------------------------------------------------------------------------*/




    INSN ( "mov",   mem16_imm8,  0x35, X_NONE )
    INSN ( "mov",   mem8_mem8,   0x45, X_NONE )
    INSN ( "mov",   mem16_mem16, 0x55, X_NONE )

    INSN ( "clrw",  X,        0x5F, X_NONE )

/*----------------------------------------------------------------------------
  Register Transfers
  ----------------------------------------------------------------------------*/

    INSN ( "exgw", X_Y,       0x51, X_NONE )

    INSN ( "swapw", X,        0x5E, X_NONE )

/*----------------------------------------------------------------------------
  Stack Operations
  ----------------------------------------------------------------------------*/

    INSN ( "pop",  A,         0x84, X_NONE )
    INSN ( "pop",  CC,        0x86, X_NONE )
    INSN ( "pop",  mem16,     0x32, X_PTR  )
    INSN ( "popw", X,         0x85, X_NONE )

    INSN ( "push", A,         0x88, X_NONE )
    INSN ( "push", CC,        0x8A, X_NONE )
    INSN ( "push", imm8,      0x4B, X_NONE )
    INSN ( "push", mem16,     0x3B, X_PTR  )
    INSN ( "pushw", X,        0x89, X_NONE )

/*----------------------------------------------------------------------------
  Logical
  ----------------------------------------------------------------------------*/

    INSN ( "tnzw", X,         0x5D, X_NONE )

/*----------------------------------------------------------------------------
  Arithmetic
  ----------------------------------------------------------------------------*/

#undef ALU_OP
#define ALU_OP(n,b) \
    INSN ( n, A_imm8,    0xA0 | b, X_NONE ) \
    INSN ( n, A_mem8,    0xB0 | b, X_PTR  ) \
    INSN ( n, A_mem16,   0xC0 | b, X_PTR  ) \
    INSN ( n, A_indX,    0xF0 | b, X_NONE ) \
    INSN ( n, A_off8_X,  0xE0 | b, X_NONE ) \
    INSN ( n, A_off16_X, 0xD0 | b, X_NONE ) \
    INSN ( n, A_off8_SP, 0x10 | b, X_NONE )

    ALU_OPS

#undef BYTE_OP
#define BYTE_OP(n,b) \
    INSN ( n, A,         0x40 | b, X_NONE ) \
    INSN ( n, mem8,      0x30 | b, X_PTR  ) \
    INSN ( n, indX,      0x70 | b, X_NONE ) \
    INSN ( n, off8_X,    0x60 | b, X_NONE ) \
    INSN ( n, off8_SP,   0x00 | b, X_NONE )

    BYTE_OPS

    INSN ( "cplw",  X,        0x53, X_NONE )
    INSN ( "decw",  X,        0x5A, X_NONE )
    INSN ( "incw",  X,        0x5C, X_NONE )

    INSN ( "mul", X_A,        0x42, X_NONE )
    INSN ( "negw",  X,        0x50, X_NONE )
    INSN ( "div", X_A,        0x62, X_NONE )


    INSN ( "divw", X_Y,       0x65, X_NONE )

/*----------------------------------------------------------------------------
  Shift and Rotate
  ----------------------------------------------------------------------------*/

    INSN ( "rrwa",  X,        0x01, X_NONE )
    INSN ( "rlwa",  X,        0x02, X_NONE )
    INSN ( "rrcw",  X,        0x56, X_NONE )
    INSN ( "rlcw",  X,        0x59, X_NONE )

    INSN ( "sraw",  X,        0x57, X_NONE )
    INSN ( "sllw",  X,        0x58, X_NONE )
    INSN ( "srlw",  X,        0x54, X_NONE )

/*----------------------------------------------------------------------------
  Jumps and Calls
  ----------------------------------------------------------------------------*/

    INSN ( "callr", rel8,     0xAD, X_CALL )
    INSN ( "callf", mem24,    0x8D, X_CALL )
    INSN ( "call",  mem16,    0xCD, X_CALL )
    INSN ( "call",  indX,     0xFD, X_CALL )
    INSN ( "call",  off8_X,   0xED, X_CALL )
    INSN ( "call",  off16_X,  0xDD, X_CALL )

    INSN ( "jp",    mem16,    0xCC, X_JMP  )
    INSN ( "jp",    indX,     0xFC, X_JMP  )
    INSN ( "jp",    off8_X,   0xEC, X_JMP  )
    INSN ( "jp",    off16_X,  0xDC, X_JMP  )

    INSN ( "jpf",   mem24,    0xAC, X_JMP  )
    INSN ( "jra",   rel8,     0x20, X_JMP  )


    INSN ( "ret",   none,     0x81, X_NONE )
    INSN ( "retf",  none,     0x87, X_NONE )

/*----------------------------------------------------------------------------
  Conditional Branch
  ----------------------------------------------------------------------------*/

    INSN ( "jrt",     rel8, 0x20, X_JMP )
    INSN ( "jrf",     rel8, 0x21, X_JMP )
    INSN ( "jrugt",   rel8, 0x22, X_JMP )
    INSN ( "jrule",   rel8, 0x23, X_JMP )
    INSN ( "jrnc",    rel8, 0x24, X_JMP )
    INSN ( "jrc",     rel8, 0x25, X_JMP )
    INSN ( "jrne",    rel8, 0x26, X_JMP )
    INSN ( "jreq",    rel8, 0x27, X_JMP )
    INSN ( "jrnv",    rel8, 0x28, X_JMP )
    INSN ( "jrv",     rel8, 0x29, X_JMP )
    INSN ( "jrpl",    rel8, 0x2A, X_JMP )
    INSN ( "jrmi",    rel8, 0x2B, X_JMP )
    INSN ( "jrsgt",   rel8, 0x2C, X_JMP )
    INSN ( "jrsle",   rel8, 0x2D, X_JMP )
    INSN ( "jrsge",   rel8, 0x2E, X_JMP )
    INSN ( "jrslt",   rel8, 0x2F, X_JMP )

/*----------------------------------------------------------------------------
  Status Flag Changes
  ----------------------------------------------------------------------------*/

    INSN ( "rvf",     none, 0x9C, X_NONE )
    INSN ( "rcf",     none, 0x98, X_NONE )
    INSN ( "ccf",     none, 0x8C, X_NONE )
    INSN ( "scf",     none, 0x99, X_NONE )
    INSN ( "rim",     none, 0x9A, X_NONE )
    INSN ( "sim",     none, 0x9B, X_NONE )

/*----------------------------------------------------------------------------
  System Functions
  ----------------------------------------------------------------------------*/

    INSN ( "break",   none, 0x8B, X_NONE )
    INSN ( "halt",    none, 0x8E, X_NONE )
    INSN ( "iret",    none, 0x80, X_NONE )
    INSN ( "nop",     none, 0x9D, X_NONE )
    INSN ( "trap",    none, 0x83, X_NONE )
    INSN ( "wfi",     none, 0x8F, X_NONE )

    INSN ( "int",    mem24,  0x82, X_JMP )

    END
};

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
