/*****************************************************************************
 *
 * Copyright (C) 2014-2019, Neil Johnson
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
    unsigned int min_cpu;
    unsigned int max_cpu;
    unsigned int min_fpu;
    const char * opcode;
    const char * (*opcode_fn)( OPC );
    void (*operands)( FILE *, ADDR *, OPC, XREF_TYPE); /* operand function */
    XREF_TYPE xtype;
    enum {
        OPTAB_UNDEF,
        OPTAB_INSN,
        OPTAB_RANGE,
        OPTAB_MASK,
        OPTAB_MASK_EXT,
        OPTAB_MASK2,
        OPTAB_MEMMOD,
        OPTAB_TABLE,
        OPTAB_PUSHTBL,
        OPTAB_PREFIX
    } type;
    union {
        struct {
            OPC min, max;
        } range;
        struct {
            OPC mask, val;
        } mask;
        struct {
            OPC mask, val;
            OPC ext_mask, ext_val;
        } mask_ext;
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

#define PREFIX(M_pfx, M_opc)         \
    { .type     = OPTAB_PREFIX,      \
      .opc      = M_opc,             \
      .opcode   = "PREFIX",          \
      .operands = prefix_ ## M_pfx,  \
    },

#define PREFIX_CPU(M_pfx, M_opc, M_min_cpu) \
    { .type     = OPTAB_PREFIX,             \
      .opc      = M_opc,                    \
      .min_cpu  = M_min_cpu,                \
      .opcode   = "PREFIX",                 \
      .operands = prefix_ ## M_pfx,         \
    },

/**
    The given instruction byte jumps to another decode table.
**/
#define TABLE(M_tablename, M_opc)    \
    { .type    = OPTAB_TABLE,        \
      .opc     = M_opc,              \
      .opcode  = "TABLE",            \
      .u.table = M_tablename         \
    },

#define TABLE_CPU(M_tablename, M_opc, M_min_cpu) \
    { .type    = OPTAB_TABLE,                    \
      .opc     = M_opc,                          \
      .min_cpu = M_min_cpu,                      \
      .opcode  = "TABLE",                        \
      .u.table = M_tablename                     \
    },

/**
    Undefined opcode, handy for exceptions in otherwise masks
    or ranges.
**/
#define UNDEF(M_opc)                 \
    { .type    = OPTAB_UNDEF,        \
      .opc     = M_opc,              \
      .opcode  = "UNDEF",            \
    },

/**
    The given instruction byte pushes M_count opcode bytes onto
    a stack and then jumps to another decode table.
**/
#define PUSHTBL(M_tablename, M_opc, M_count)    \
    { .type    = OPTAB_PUSHTBL,      \
      .opc     = M_opc,              \
      .opcode  = "PUSHTBL",          \
      .u.pushtbl.table = M_tablename,\
      .u.pushtbl.n     = M_count     \
    },
    
/**
    A single instruction matches against one op byte.
**/    
#define INSN(M_opcode, M_ops, M_opc, M_xt)  \
    { .type     = OPTAB_INSN,               \
      .opc      = M_opc,                    \
      .opcode   = M_opcode,                 \
      .operands = operand_ ## M_ops,        \
      .xtype    = M_xt                      \
    },

#define INSN_CPU(M_opcode, M_ops, M_opc, M_xt, M_min_cpu)  \
    { .type     = OPTAB_INSN,                              \
      .opc      = M_opc,                                   \
      .min_cpu  = M_min_cpu,                               \
      .opcode   = M_opcode,                                \
      .operands = operand_ ## M_ops,                       \
      .xtype    = M_xt                                     \
    },

#define INSN_DYN(M_opcode_fn, M_ops, M_opc, M_xt)  \
    { .type      = OPTAB_INSN,                     \
      .opc       = M_opc,                          \
      .opcode    = "DYNAMIC",                      \
      .opcode_fn = opcode_ ## M_opcode_fn,         \
      .operands  = operand_ ## M_ops,              \
      .xtype     = M_xt                            \
    },

#define INSN_DYN_CPU(M_opcode_fn, M_ops, M_opc, M_xt, M_min_cpu) \
    { .type      = OPTAB_INSN,                                  \
      .opc       = M_opc,                                       \
      .min_cpu   = M_min_cpu,                                   \
      .opcode    = "DYNAMIC",                                   \
      .opcode_fn = opcode_ ## M_opcode_fn,                      \
      .operands  = operand_ ## M_ops,                           \
      .xtype     = M_xt                                         \
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

#define RANGE_DYN(M_opcode_fn, M_ops, M_min, M_max, M_xt)  \
    { .type      = OPTAB_RANGE,                            \
      .opcode    = "DYNAMIC",                              \
      .opcode_fn = opcode_ ## M_opcode_fn,                 \
      .operands  = operand_ ## M_ops,                      \
      .xtype     = M_xt,                                   \
      .u.range.min = M_min,                                \
      .u.range.max = M_max                                 \
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

#define MASK_CPU(M_opcode, M_ops, M_mask, M_val, M_xt, M_min_cpu)  \
    { .type     = OPTAB_MASK,                                      \
      .min_cpu  = M_min_cpu,                                       \
      .opcode   = M_opcode,                                        \
      .operands = operand_ ## M_ops,                               \
      .xtype    = M_xt,                                            \
      .u.mask.mask = M_mask,                                       \
      .u.mask.val  = M_val                                         \
    },

#define MASK_FPU(M_opcode, M_ops, M_mask, M_val, M_xt, M_min_fpu)  \
    { .type     = OPTAB_MASK,                                      \
      .min_fpu  = M_min_fpu,                                       \
      .opcode   = M_opcode,                                        \
      .operands = operand_ ## M_ops,                               \
      .xtype    = M_xt,                                            \
      .u.mask.mask = M_mask,                                       \
      .u.mask.val  = M_val                                         \
    },

#define MASK_EXT_FPU(M_opcode, M_ops, M_mask, M_val, M_ext_mask, M_ext_val, M_xt, M_min_fpu) \
    { .type     = OPTAB_MASK_EXT,                                                       \
      .min_fpu  = M_min_fpu,                                                            \
      .opcode   = M_opcode,                                                             \
      .operands = operand_ ## M_ops,                                                     \
      .xtype    = M_xt,                                                                  \
      .u.mask_ext.mask = M_mask,                                                         \
      .u.mask_ext.val  = M_val,                                                          \
      .u.mask_ext.ext_mask = M_ext_mask,                                                 \
      .u.mask_ext.ext_val  = M_ext_val                                                   \
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

#define MASK2_CPU(M_opcode, M_ops, M_opc, M_mask, M_val, M_xt, M_min_cpu) \
    { .type     = OPTAB_MASK2,                                            \
      .opcode   = M_opcode,                                               \
      .opc      = M_opc,                                                  \
      .min_cpu  = M_min_cpu,                                              \
      .operands = operand_ ## M_ops,                                      \
      .xtype    = M_xt,                                                   \
      .u.mask.mask = M_mask,                                              \
      .u.mask.val  = M_val                                                \
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
    Create operand function definition given a name.
**/
#define OPERAND_FUNC(M_name) \
    static void operand_ ## M_name (FILE *f, ADDR * addr, OPC opc, XREF_TYPE xtype )
    
/**
    Create prefix function definition given a name.
**/
#define PREFIX_FUNC(M_name) \
    static void prefix_ ## M_name (FILE *f, ADDR * addr, OPC opc, XREF_TYPE xtype )

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
    Generate a pair of two-operand functions: "A,B" and "B,A"
**/
#define TWO_OPERAND_PAIR(P_a, P_b) \
TWO_OPERAND(P_a, P_b) \
TWO_OPERAND(P_b, P_a)

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

/* Optional target-specific instruction hooks.  optab.c provides weak no-op
 * defaults for decoders that do not need per-instruction state.
 */
extern void dasm_pre_insn( void );
extern void dasm_post_insn( void );

#endif /* _OPTAB_H_ */
