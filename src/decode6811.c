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

DASM_PROFILE( "dasm6811", "Motorola 68HC11", 5, 8, 1, 1, 1 )

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/

#define FORMAT_NUM_8BIT         "$%02X"
#define FORMAT_NUM_16BIT        "$%04X"

#define MK_WORD(l,h)            ( ((l) & 0xFF) | (((h) & 0xFF) << 8) )

int dasm_cfg_supported( void )
{
    return 1;
}

static int read_opcode_byte( ADDR addr, UBYTE *out )
{
    return dasm_input_read_byte_at( addr, out );
}

void dasm_post_insn( void )
{
    UBYTE op0, op1;

    if ( !read_opcode_byte( g_insn_addr, &op0 ) )
        return;

    if ( op0 == 0x18 )
    {
        if ( !read_opcode_byte( g_insn_addr + 1, &op1 ) )
            return;

        if ( op1 == 0x1E || op1 == 0x1F )
            dasm_cfg_set_flow( CFG_FLOW_COND_JUMP );
        else if ( op1 == 0x6E )
            dasm_cfg_set_flow( CFG_FLOW_INDIRECT_JUMP );
        else if ( op1 == 0xAD )
            dasm_cfg_set_flow( CFG_FLOW_INDIRECT_CALL );
        return;
    }

    if ( op0 == 0x12 || op0 == 0x13 || op0 == 0x1E || op0 == 0x1F )
        dasm_cfg_set_flow( CFG_FLOW_COND_JUMP );
    else if ( op0 == 0x20 )
        dasm_cfg_set_flow( CFG_FLOW_JUMP );
    else if ( op0 >= 0x22 && op0 <= 0x2F )
        dasm_cfg_set_flow( CFG_FLOW_COND_JUMP );
    else if ( op0 == 0x39 || op0 == 0x3B )
        dasm_cfg_set_flow( CFG_FLOW_RETURN );
    else if ( op0 == 0x3E )
        dasm_cfg_set_flow( CFG_FLOW_HALT );
    else if ( op0 == 0x3F )
        dasm_cfg_set_flow( CFG_FLOW_INDIRECT_CALL );
    else if ( op0 == 0x7E )
        dasm_cfg_set_flow( CFG_FLOW_JUMP );
    else if ( op0 == 0x6E )
        dasm_cfg_set_flow( CFG_FLOW_INDIRECT_JUMP );
    else if ( op0 == 0x8D || op0 == 0x9D || op0 == 0xBD )
        dasm_cfg_set_flow( CFG_FLOW_CALL );
    else if ( op0 == 0xAD )
        dasm_cfg_set_flow( CFG_FLOW_INDIRECT_CALL );
    else if ( op0 == 0xCF )
        dasm_cfg_set_flow( CFG_FLOW_STOP );
}

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

OPERAND_FUNC(indexed_y)
{
    UBYTE offset = next( f, addr );

    operand( FORMAT_NUM_8BIT ",y", offset );
}

OPERAND_FUNC(rel8)
{
    BYTE disp = (BYTE)next( f, addr );
    ADDR dest = *addr + disp;

    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( xtype, g_insn_addr, dest );
}

OPERAND_FUNC(bit_direct)
{
    UBYTE a = next( f, addr );
    UBYTE mask = next( f, addr );

    operand( FORMAT_NUM_8BIT ",#" FORMAT_NUM_8BIT, a, mask );
    xref_addxref( xtype, g_insn_addr, a );
}

OPERAND_FUNC(bit_indexed)
{
    UBYTE offset = next( f, addr );
    UBYTE mask = next( f, addr );

    operand( FORMAT_NUM_8BIT ",x,#" FORMAT_NUM_8BIT, offset, mask );
}

OPERAND_FUNC(bit_indexed_y)
{
    UBYTE offset = next( f, addr );
    UBYTE mask = next( f, addr );

    operand( FORMAT_NUM_8BIT ",y,#" FORMAT_NUM_8BIT, offset, mask );
}

OPERAND_FUNC(brbit_direct)
{
    UBYTE a = next( f, addr );
    UBYTE mask = next( f, addr );
    BYTE disp = (BYTE)next( f, addr );
    ADDR dest = *addr + disp;

    operand( FORMAT_NUM_8BIT ",#" FORMAT_NUM_8BIT ",", a, mask );
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( X_JMP, g_insn_addr, dest );
}

OPERAND_FUNC(brbit_indexed)
{
    UBYTE offset = next( f, addr );
    UBYTE mask = next( f, addr );
    BYTE disp = (BYTE)next( f, addr );
    ADDR dest = *addr + disp;

    operand( FORMAT_NUM_8BIT ",x,#" FORMAT_NUM_8BIT ",", offset, mask );
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( X_JMP, g_insn_addr, dest );
}

OPERAND_FUNC(brbit_indexed_y)
{
    UBYTE offset = next( f, addr );
    UBYTE mask = next( f, addr );
    BYTE disp = (BYTE)next( f, addr );
    ADDR dest = *addr + disp;

    operand( FORMAT_NUM_8BIT ",y,#" FORMAT_NUM_8BIT ",", offset, mask );
    operand( xref_genwordaddr( NULL, FORMAT_NUM_16BIT, dest ) );
    xref_addxref( X_JMP, g_insn_addr, dest );
}

extern optab_t y_optab[];
extern optab_t cpd_x_optab[];
extern optab_t cpd_y_optab[];

optab_t base_optab[] = {
    /*
     * Control and condition-code instructions.
     */
    INSN ( "test", none, 0x00, X_NONE )
    INSN ( "nop",  none, 0x01, X_NONE )
    INSN ( "idiv", none, 0x02, X_NONE )
    INSN ( "fdiv", none, 0x03, X_NONE )
    INSN ( "lsrd", none, 0x04, X_NONE )
    INSN ( "asld", none, 0x05, X_NONE )
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
    INSN ( "brset", brbit_direct, 0x12, X_JMP )
    INSN ( "brclr", brbit_direct, 0x13, X_JMP )
    INSN ( "bset",  bit_direct,   0x14, X_DIRECT )
    INSN ( "bclr",  bit_direct,   0x15, X_DIRECT )
    INSN ( "tab",  none, 0x16, X_NONE )
    INSN ( "tba",  none, 0x17, X_NONE )
    TABLE( y_optab,     0x18 )
    INSN ( "daa",  none, 0x19, X_NONE )
    TABLE( cpd_x_optab, 0x1A )
    INSN ( "aba",  none, 0x1B, X_NONE )
    INSN ( "bset",  bit_indexed,   0x1C, X_PTR )
    INSN ( "bclr",  bit_indexed,   0x1D, X_PTR )
    INSN ( "brset", brbit_indexed, 0x1E, X_JMP )
    INSN ( "brclr", brbit_indexed, 0x1F, X_JMP )
    INSN ( "tsx",  none, 0x30, X_NONE )
    INSN ( "ins",  none, 0x31, X_NONE )
    INSN ( "pula", none, 0x32, X_NONE )
    INSN ( "pulb", none, 0x33, X_NONE )
    INSN ( "des",  none, 0x34, X_NONE )
    INSN ( "txs",  none, 0x35, X_NONE )
    INSN ( "psha", none, 0x36, X_NONE )
    INSN ( "pshb", none, 0x37, X_NONE )
    INSN ( "pulx", none, 0x38, X_NONE )
    INSN ( "rts",  none, 0x39, X_NONE )
    INSN ( "abx",  none, 0x3A, X_NONE )
    INSN ( "rti",  none, 0x3B, X_NONE )
    INSN ( "pshx", none, 0x3C, X_NONE )
    INSN ( "mul",  none, 0x3D, X_NONE )
    INSN ( "wai",  none, 0x3E, X_NONE )
    INSN ( "swi",  none, 0x3F, X_NONE )

    /*
     * Branches.
     */
    INSN ( "bra", rel8, 0x20, X_JMP )
    INSN ( "brn", rel8, 0x21, X_JMP )
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
    INSN ( "subd", imm16,    0x83, X_IMM )
    INSN ( "subd", direct,   0x93, X_DIRECT )
    INSN ( "subd", indexed,  0xA3, X_PTR )
    INSN ( "subd", extended, 0xB3, X_PTR )

    INSN ( "cpx", imm16,    0x8C, X_IMM )
    INSN ( "cpx", direct,   0x9C, X_DIRECT )
    INSN ( "cpx", indexed,  0xAC, X_PTR )
    INSN ( "cpx", extended, 0xBC, X_PTR )

    INSN ( "jsr", direct, 0x9D, X_CALL )
    INSN ( "jsr", indexed,  0xAD, X_PTR )
    INSN ( "jsr", extended, 0xBD, X_CALL )

    INSN ( "lds", imm16,    0x8E, X_IMM )
    INSN ( "lds", direct,   0x9E, X_DIRECT )
    INSN ( "lds", indexed,  0xAE, X_PTR )
    INSN ( "lds", extended, 0xBE, X_PTR )
    INSN ( "sts", direct,   0x9F, X_DIRECT )
    INSN ( "sts", indexed,  0xAF, X_PTR )
    INSN ( "sts", extended, 0xBF, X_PTR )
    INSN ( "xgdx", none, 0x8F, X_NONE )

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

    INSN ( "addd", imm16,    0xC3, X_IMM )
    INSN ( "addd", direct,   0xD3, X_DIRECT )
    INSN ( "addd", indexed,  0xE3, X_PTR )
    INSN ( "addd", extended, 0xF3, X_PTR )
    INSN ( "ldd",  imm16,    0xCC, X_IMM )
    TABLE( cpd_y_optab, 0xCD )
    INSN ( "stop", none,     0xCF, X_NONE )
    INSN ( "ldd",  direct,   0xDC, X_DIRECT )
    INSN ( "ldd",  indexed,  0xEC, X_PTR )
    INSN ( "ldd",  extended, 0xFC, X_PTR )
    INSN ( "std",  direct,   0xDD, X_DIRECT )
    INSN ( "std",  indexed,  0xED, X_PTR )
    INSN ( "std",  extended, 0xFD, X_PTR )

    INSN ( "ldx", imm16,    0xCE, X_IMM )
    INSN ( "ldx", direct,   0xDE, X_DIRECT )
    INSN ( "ldx", indexed,  0xEE, X_PTR )
    INSN ( "ldx", extended, 0xFE, X_PTR )
    INSN ( "stx", direct,   0xDF, X_DIRECT )
    INSN ( "stx", indexed,  0xEF, X_PTR )
    INSN ( "stx", extended, 0xFF, X_PTR )

    END
};

optab_t y_optab[] = {
    INSN ( "iny", none, 0x08, X_NONE )
    INSN ( "dey", none, 0x09, X_NONE )
    INSN ( "bset",  bit_indexed_y,   0x1C, X_PTR )
    INSN ( "bclr",  bit_indexed_y,   0x1D, X_PTR )
    INSN ( "brset", brbit_indexed_y, 0x1E, X_JMP )
    INSN ( "brclr", brbit_indexed_y, 0x1F, X_JMP )
    INSN ( "tsy", none, 0x30, X_NONE )
    INSN ( "tys", none, 0x35, X_NONE )
    INSN ( "aby", none, 0x3A, X_NONE )

#define RMW_OP_Y(M_name, M_base) \
    INSN ( M_name, indexed_y, ( 0x60 | M_base ), X_PTR )

    RMW_OP_Y( "neg", 0x00 )
    RMW_OP_Y( "com", 0x03 )
    RMW_OP_Y( "lsr", 0x04 )
    RMW_OP_Y( "ror", 0x06 )
    RMW_OP_Y( "asr", 0x07 )
    RMW_OP_Y( "asl", 0x08 )
    RMW_OP_Y( "rol", 0x09 )
    RMW_OP_Y( "dec", 0x0A )
    RMW_OP_Y( "inc", 0x0C )
    RMW_OP_Y( "tst", 0x0D )
    RMW_OP_Y( "clr", 0x0F )
    INSN ( "jmp", indexed_y, 0x6E, X_PTR )

#define REGA_OP_Y(M_name, M_base) \
    INSN ( M_name, indexed_y, ( 0xA0 | M_base ), X_PTR )

    REGA_OP_Y( "suba", 0x00 )
    REGA_OP_Y( "cmpa", 0x01 )
    REGA_OP_Y( "sbca", 0x02 )
    REGA_OP_Y( "anda", 0x04 )
    REGA_OP_Y( "bita", 0x05 )
    REGA_OP_Y( "ldaa", 0x06 )
    INSN ( "staa", indexed_y, 0xA7, X_PTR )
    REGA_OP_Y( "eora", 0x08 )
    REGA_OP_Y( "adca", 0x09 )
    REGA_OP_Y( "oraa", 0x0A )
    REGA_OP_Y( "adda", 0x0B )

    INSN ( "subd", indexed_y, 0xA3, X_PTR )
    INSN ( "cpy",  imm16,     0x8C, X_IMM )
    INSN ( "cpy",  direct,    0x9C, X_DIRECT )
    INSN ( "cpy",  indexed_y, 0xAC, X_PTR )
    INSN ( "cpy",  extended,  0xBC, X_PTR )
    INSN ( "jsr",  indexed_y, 0xAD, X_PTR )
    INSN ( "lds",  indexed_y, 0xAE, X_PTR )
    INSN ( "sts",  indexed_y, 0xAF, X_PTR )
    INSN ( "xgdy", none,      0x8F, X_NONE )

#define REGB_OP_Y(M_name, M_base) \
    INSN ( M_name, indexed_y, ( 0xE0 | M_base ), X_PTR )

    REGB_OP_Y( "subb", 0x00 )
    REGB_OP_Y( "cmpb", 0x01 )
    REGB_OP_Y( "sbcb", 0x02 )
    REGB_OP_Y( "andb", 0x04 )
    REGB_OP_Y( "bitb", 0x05 )
    REGB_OP_Y( "ldab", 0x06 )
    INSN ( "stab", indexed_y, 0xE7, X_PTR )
    REGB_OP_Y( "eorb", 0x08 )
    REGB_OP_Y( "adcb", 0x09 )
    REGB_OP_Y( "orab", 0x0A )
    REGB_OP_Y( "addb", 0x0B )

    INSN ( "addd", indexed_y, 0xE3, X_PTR )
    INSN ( "ldd",  indexed_y, 0xEC, X_PTR )
    INSN ( "std",  indexed_y, 0xED, X_PTR )
    INSN ( "ldy",  imm16,     0xCE, X_IMM )
    INSN ( "ldy",  direct,    0xDE, X_DIRECT )
    INSN ( "sty",  direct,    0xDF, X_DIRECT )
    INSN ( "ldy",  indexed_y, 0xEE, X_PTR )
    INSN ( "sty",  indexed_y, 0xEF, X_PTR )
    INSN ( "ldy",  extended,  0xFE, X_PTR )
    INSN ( "sty",  extended,  0xFF, X_PTR )

    END
};

optab_t cpd_x_optab[] = {
    INSN ( "cpd", imm16,    0x83, X_IMM )
    INSN ( "cpd", direct,   0x93, X_DIRECT )
    INSN ( "cpd", indexed,  0xA3, X_PTR )
    INSN ( "cpd", extended, 0xB3, X_PTR )

    END
};

optab_t cpd_y_optab[] = {
    INSN ( "cpd", indexed_y, 0xA3, X_PTR )

    END
};
