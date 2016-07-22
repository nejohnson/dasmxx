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
 
#ifndef _OPTAB_H_
#define _OPTAB_H_

#include "dasmxx.h"

/**
    The optab_t type describes each entry in the op tables.
**/
typedef struct optab_s {
    OPC opc;
    const char * opcode;
    void (*operands)( FILE *, ADDR *, UBYTE, XREF_TYPE); /* operand function */
    XREF_TYPE xtype;
    enum {
        OPTAB_INSN,
        OPTAB_RANGE,
        OPTAB_MASK,
        OPTAB_MASK2,
        OPTAB_MEMMOD,
        OPTAB_TABLE,
        OPTAB_PUSHTBL
    } type;
    union {
        struct {
            OPC min, max;
        } range;
        struct {
            OPC mask, val;
        } mask;
        struct optab_s * table;
        struct {
            struct optab_s * table;
            int n;
	} pushtbl;
    } u;
} optab_t;

/**
    Macros to construct entries in op tables.
**/

/**
    The given instruction byte jumps to another decode table.
**/
#define TABLE(M_tablename, M_opc)    \
    { .type    = OPTAB_TABLE,        \
      .opc     = M_opc,              \
      .opcode  = "TABLE",            \
      .u.table = M_tablename         \
    },

/**
    The given instruction byte pushes M_count opcode bytes onto
    a stack ans then jumps to another decode table.
**/
#define PUSHTBL(M_tablename, M_opc, M_count)    \
    { .type    = OPTAB_PUSHTBL,      \
      .opc     = M_opc,              \
      .opcode  = "PUSHTBL",          \
      .u.pushtbl.table = M_tablename,\
      .u.pushtbl.n     = M_count     \
    },
    
/**
    A single insruction matches against one op byte.
**/    
#define INSN(M_opcode, M_ops, M_opc, M_xt)  \
    { .type     = OPTAB_INSN,               \
      .opc      = M_opc,                    \
      .opcode   = M_opcode,                 \
      .operands = operand_ ## M_ops,        \
      .xtype    = M_xt                      \
    },

/**
    A RANGE matches the first byte anywhere between M_min and M_max inclusive.
**/    
#define RANGE(M_opcode, M_ops, M_min, M_max, M_xt)  \
    { .type     = OPTAB_RANGE,                      \
      .opcode   = M_opcode,                         \
      .operands = operand_ ## M_ops,                \
      .xtype    = M_xt,                             \
      .u.range.min = M_min,                         \
      .u.range.max = M_max                          \
    },

/**
    A MASK matches a set of instruction bytes described by a bit mask and a
   value to match against applied to the first search byte.
**/    
#define MASK(M_opcode, M_ops, M_mask, M_val, M_xt)  \
    { .type     = OPTAB_MASK,                       \
      .opcode   = M_opcode,                         \
      .operands = operand_ ## M_ops,                \
      .xtype    = M_xt,                             \
      .u.mask.mask = M_mask,                        \
      .u.mask.val  = M_val                          \
    },

/**
    A MASK2 matches a set of instruction bytes described by a bit mask and a
   value to match against applied to the second search byte.
**/    
#define MASK2(M_opcode, M_ops, M_opc, M_mask, M_val, M_xt)  \
    { .type     = OPTAB_MASK2,                              \
      .opcode   = M_opcode,                                 \
      .opc      = M_opc,                                    \
      .operands = operand_ ## M_ops,                        \
      .xtype    = M_xt,                                     \
      .u.mask.mask = M_mask,                                \
      .u.mask.val  = M_val                                  \
    },
                                                            
/**
    A MEMMOD describes an instruction with four memory-modifer combinations
    which must be decoded together for a prospective match.
**/    
#define MEMMOD(M_opcode, M_ops, M_opc, M_xt)    \
    { .type     = OPTAB_MEMMOD,                 \
      .opcode   = M_opcode,                     \
      .opc      = M_opc,                        \
      .operands = operand_ ## M_ops,            \
      .xtype    = M_xt                          \
    },
                                                            
/**
    Mark end of op table.
**/
#define END        { .opcode = NULL }

/**
    Create function definition given a name.
**/
#define OPERAND_FUNC(M_name) \
    static void operand_ ## M_name (FILE *f, ADDR * addr, UBYTE opc, XREF_TYPE xtype )

/* Neaten up emitting a comma "," within an operand. */
#define COMMA                   operand( ", " )

/**
    Short-cut macro to generate simple two-operand functions.
**/
#define TWO_OPERAND(M_a,M_b) \
OPERAND_FUNC(M_a ## _ ## M_b) \
{ \
      operand_ ## M_a (f, addr, opc, xtype); \
      COMMA; \
      operand_ ## M_b (f, addr, opc, xtype); \
}

/**
    Short-cut macro to generate simple three-operand functions.
**/
#define THREE_OPERAND(M_a,M_b,M_c) \
OPERAND_FUNC(M_a ## _ ## M_b ## _ ## M_c) \
{ \
      operand_ ## M_a (f, addr, opc, xtype); \
      COMMA; \
      operand_ ## M_b (f, addr, opc, xtype); \
      COMMA; \
      operand_ ## M_c (f, addr, opc, xtype); \
}

/* Create a single-bit mask */
#define BIT(n)                  ( 1 << (n) )

/* General function for outputting an operand */
extern void operand( const char * operand, ... );

/* Push and pop opcodes to an internal stack */
extern void stack_push( OPC );
extern OPC  stack_pop( void );

/* Start address of each instruction as it is decoded. */
extern ADDR g_insn_addr;

#endif /* _OPTAB_H_ */

