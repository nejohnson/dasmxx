/*****************************************************************************
 *
 * Copyright (C) 2026, Neil Johnson
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

#include "dasmxx.h"
#include "optab.h"

/*****************************************************************************
 * Globally-visible decoder properties
 *****************************************************************************/

DASM_PROFILE( "dasm6800", "Motorola 6800/6801/6803", 3, 8, 1, 1, 1 )

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/

#define FORMAT_NUM_8BIT         "$%02X"
#define FORMAT_NUM_16BIT        "$%04X"

#define MK_WORD(l,h)            ( ((l) & 0xFF) | (((h) & 0xFF) << 8) )

/*****************************************************************************
 *        Private Functions
 *****************************************************************************/

OPERAND_FUNC(none)
{
    /* empty */
}

OPERAND_FUNC(imm8)
{
    UBYTE byte = next( f, addr );

    operand( "#" FORMAT_NUM_8BIT, byte );
}

OPERAND_FUNC(imm16)
{
    UBYTE msb = next( f, addr );
    UBYTE lsb = next( f, addr );
    UWORD imm16 = MK_WORD( lsb, msb );

    operand( "#" FORMAT_NUM_16BIT, imm16 );
}

OPERAND_FUNC(direct)
{
    UBYTE a = next( f, addr );

    if ( xtype == X_JMP || xtype == X_CALL )
        operand( xref_genwordaddr( NULL, FORMAT_NUM_8BIT, (ADDR)a ) );
    else
        operand( FORMAT_NUM_8BIT, a );
    xref_addxref( xtype, g_insn_addr, a );
}

OPERAND_FUNC(extended)
{
    UBYTE msb = next( f, addr );
    UBYTE lsb = next( f, addr );
    UWORD addr16 = MK_WORD( lsb, msb );

    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, addr16 ) );
    xref_addxref( xtype, g_insn_addr, addr16 );
}

OPERAND_FUNC(indexed)
{
    UBYTE offset = next( f, addr );

    operand( FORMAT_NUM_8BIT ",x", offset );
}

OPERAND_FUNC(rel8)
{
    BYTE disp = (BYTE)next( f, addr );
    ADDR dest = *addr + disp;

    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

optab_t base_optab[] = {
    /*
     * Control and condition-code instructions.
     */
    INSN ( "nop",  none, 0x01, X_NONE )
    INSN_CPU ( "lsrd", none, 0x04, X_NONE, 6801 )
    INSN_CPU ( "asld", none, 0x05, X_NONE, 6801 )
    INSN ( "tap",  none, 0x06, X_NONE )
    INSN ( "tpa",  none, 0x07, X_NONE )
    INSN ( "inx",  none, 0x08, X_NONE )
    INSN ( "dex",  none, 0x09, X_NONE )
    INSN ( "clv",  none, 0x0A, X_NONE )
    INSN ( "sev",  none, 0x0B, X_NONE )
    INSN ( "clc",  none, 0x0C, X_NONE )
    INSN ( "sec",  none, 0x0D, X_NONE )
    INSN ( "cli",  none, 0x0E, X_NONE )
    INSN ( "sei",  none, 0x0F, X_NONE )
    INSN ( "sba",  none, 0x10, X_NONE )
    INSN ( "cba",  none, 0x11, X_NONE )
    INSN ( "tab",  none, 0x16, X_NONE )
    INSN ( "tba",  none, 0x17, X_NONE )
    INSN_CPU ( "slp",  none, 0x1A, X_NONE, 6801 )
    INSN ( "daa",  none, 0x19, X_NONE )
    INSN ( "aba",  none, 0x1B, X_NONE )
    INSN ( "tsx",  none, 0x30, X_NONE )
    INSN ( "ins",  none, 0x31, X_NONE )
    INSN ( "pula", none, 0x32, X_NONE )
    INSN ( "pulb", none, 0x33, X_NONE )
    INSN ( "des",  none, 0x34, X_NONE )
    INSN ( "txs",  none, 0x35, X_NONE )
    INSN ( "psha", none, 0x36, X_NONE )
    INSN ( "pshb", none, 0x37, X_NONE )
    INSN_CPU ( "pulx", none, 0x38, X_NONE, 6801 )
    INSN ( "rts",  none, 0x39, X_NONE )
    INSN_CPU ( "abx",  none, 0x3A, X_NONE, 6801 )
    INSN ( "rti",  none, 0x3B, X_NONE )
    INSN_CPU ( "pshx", none, 0x3C, X_NONE, 6801 )
    INSN_CPU ( "mul",  none, 0x3D, X_NONE, 6801 )
    INSN ( "wai",  none, 0x3E, X_NONE )
    INSN ( "swi",  none, 0x3F, X_NONE )

    /*
     * Branches.
     */
    INSN ( "bra", rel8, 0x20, X_JMP )
    INSN_CPU ( "brn", rel8, 0x21, X_JMP, 6801 )
    INSN ( "bhi", rel8, 0x22, X_JMP )
    INSN ( "bls", rel8, 0x23, X_JMP )
    INSN ( "bcc", rel8, 0x24, X_JMP )
    INSN ( "bcs", rel8, 0x25, X_JMP )
    INSN ( "bne", rel8, 0x26, X_JMP )
    INSN ( "beq", rel8, 0x27, X_JMP )
    INSN ( "bvc", rel8, 0x28, X_JMP )
    INSN ( "bvs", rel8, 0x29, X_JMP )
    INSN ( "bpl", rel8, 0x2A, X_JMP )
    INSN ( "bmi", rel8, 0x2B, X_JMP )
    INSN ( "bge", rel8, 0x2C, X_JMP )
    INSN ( "blt", rel8, 0x2D, X_JMP )
    INSN ( "bgt", rel8, 0x2E, X_JMP )
    INSN ( "ble", rel8, 0x2F, X_JMP )

    /*
     * Read/modify/write instructions.
     */
#define RMW_OP(M_name, M_base) \
    INSN ( M_name "a", none,     ( 0x40 | M_base ), X_NONE ) \
    INSN ( M_name "b", none,     ( 0x50 | M_base ), X_NONE ) \
    INSN ( M_name,      indexed,  ( 0x60 | M_base ), X_PTR )  \
    INSN ( M_name,      extended, ( 0x70 | M_base ), X_PTR )

    RMW_OP( "neg", 0x00 )
    RMW_OP( "com", 0x03 )
    RMW_OP( "lsr", 0x04 )
    RMW_OP( "ror", 0x06 )
    RMW_OP( "asr", 0x07 )
    RMW_OP( "asl", 0x08 )
    RMW_OP( "rol", 0x09 )
    RMW_OP( "dec", 0x0A )
    RMW_OP( "inc", 0x0C )
    RMW_OP( "tst", 0x0D )
    RMW_OP( "clr", 0x0F )
    INSN ( "jmp", indexed,  0x6E, X_PTR )
    INSN ( "jmp", extended, 0x7E, X_JMP )

    /*
     * Accumulator A register/memory instructions.
     */
#define REGA_OP(M_name, M_base) \
    INSN ( M_name, imm8,     ( 0x80 | M_base ), X_IMM )    \
    INSN ( M_name, direct,   ( 0x90 | M_base ), X_DIRECT ) \
    INSN ( M_name, indexed,  ( 0xA0 | M_base ), X_PTR )    \
    INSN ( M_name, extended, ( 0xB0 | M_base ), X_PTR )

    REGA_OP( "suba", 0x00 )
    REGA_OP( "cmpa", 0x01 )
    REGA_OP( "sbca", 0x02 )
    REGA_OP( "anda", 0x04 )
    REGA_OP( "bita", 0x05 )
    REGA_OP( "ldaa", 0x06 )
    INSN ( "staa", direct,   0x97, X_DIRECT )
    INSN ( "staa", indexed,  0xA7, X_PTR )
    INSN ( "staa", extended, 0xB7, X_PTR )
    REGA_OP( "eora", 0x08 )
    REGA_OP( "adca", 0x09 )
    REGA_OP( "oraa", 0x0A )
    REGA_OP( "adda", 0x0B )

    INSN ( "bsr", rel8, 0x8D, X_CALL )
    INSN_CPU ( "subd", imm16,    0x83, X_IMM,    6801 )
    INSN_CPU ( "subd", direct,   0x93, X_DIRECT, 6801 )
    INSN_CPU ( "subd", indexed,  0xA3, X_PTR,    6801 )
    INSN_CPU ( "subd", extended, 0xB3, X_PTR,    6801 )

    INSN ( "cpx", imm16,    0x8C, X_IMM )
    INSN ( "cpx", direct,   0x9C, X_DIRECT )
    INSN ( "cpx", indexed,  0xAC, X_PTR )
    INSN ( "cpx", extended, 0xBC, X_PTR )

    INSN_CPU ( "jsr", direct, 0x9D, X_CALL, 6801 )
    INSN ( "jsr", indexed,  0xAD, X_PTR )
    INSN ( "jsr", extended, 0xBD, X_CALL )

    INSN ( "lds", imm16,    0x8E, X_IMM )
    INSN ( "lds", direct,   0x9E, X_DIRECT )
    INSN ( "lds", indexed,  0xAE, X_PTR )
    INSN ( "lds", extended, 0xBE, X_PTR )
    INSN ( "sts", direct,   0x9F, X_DIRECT )
    INSN ( "sts", indexed,  0xAF, X_PTR )
    INSN ( "sts", extended, 0xBF, X_PTR )
    INSN_CPU ( "xgdx", none, 0x8F, X_NONE, 6801 )

    /*
     * Accumulator B register/memory instructions.
     */
#define REGB_OP(M_name, M_base) \
    INSN ( M_name, imm8,     ( 0xC0 | M_base ), X_IMM )    \
    INSN ( M_name, direct,   ( 0xD0 | M_base ), X_DIRECT ) \
    INSN ( M_name, indexed,  ( 0xE0 | M_base ), X_PTR )    \
    INSN ( M_name, extended, ( 0xF0 | M_base ), X_PTR )

    REGB_OP( "subb", 0x00 )
    REGB_OP( "cmpb", 0x01 )
    REGB_OP( "sbcb", 0x02 )
    REGB_OP( "andb", 0x04 )
    REGB_OP( "bitb", 0x05 )
    REGB_OP( "ldab", 0x06 )
    INSN ( "stab", direct,   0xD7, X_DIRECT )
    INSN ( "stab", indexed,  0xE7, X_PTR )
    INSN ( "stab", extended, 0xF7, X_PTR )
    REGB_OP( "eorb", 0x08 )
    REGB_OP( "adcb", 0x09 )
    REGB_OP( "orab", 0x0A )
    REGB_OP( "addb", 0x0B )

    INSN_CPU ( "addd", imm16,    0xC3, X_IMM,    6801 )
    INSN_CPU ( "addd", direct,   0xD3, X_DIRECT, 6801 )
    INSN_CPU ( "addd", indexed,  0xE3, X_PTR,    6801 )
    INSN_CPU ( "addd", extended, 0xF3, X_PTR,    6801 )
    INSN_CPU ( "ldd",  imm16,    0xCC, X_IMM,    6801 )
    INSN_CPU ( "ldd",  direct,   0xDC, X_DIRECT, 6801 )
    INSN_CPU ( "ldd",  indexed,  0xEC, X_PTR,    6801 )
    INSN_CPU ( "ldd",  extended, 0xFC, X_PTR,    6801 )
    INSN_CPU ( "std",  direct,   0xDD, X_DIRECT, 6801 )
    INSN_CPU ( "std",  indexed,  0xED, X_PTR,    6801 )
    INSN_CPU ( "std",  extended, 0xFD, X_PTR,    6801 )

    INSN ( "ldx", imm16,    0xCE, X_IMM )
    INSN ( "ldx", direct,   0xDE, X_DIRECT )
    INSN ( "ldx", indexed,  0xEE, X_PTR )
    INSN ( "ldx", extended, 0xFE, X_PTR )
    INSN ( "stx", direct,   0xDF, X_DIRECT )
    INSN ( "stx", indexed,  0xEF, X_PTR )
    INSN ( "stx", extended, 0xFF, X_PTR )

    END
};
