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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "dasmxx.h"
#include "optab.h"

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/

/* Indicate whether decode was successful or not. */
#define INSN_FOUND              ( 1 )
#define INSN_NOT_FOUND          ( 0 )

/*****************************************************************************
 * External data.
 *****************************************************************************/

extern optab_t base_optab[];

/*****************************************************************************
 * Global data. Delcare as extern in header file.
 *****************************************************************************/

/* Start address of each instruction as it is decoded. */
ADDR g_insn_addr = 0;

/*****************************************************************************
 * Private data.
 *****************************************************************************/

/* Global output buffer into which the decoded output is written. */
static char * output_buffer = NULL; 

/*****************************************************************************
 *        Private Functions
 *****************************************************************************/

/***********************************************************
 *
 * FUNCTION
 *      opcode
 *
 * DESCRIPTION
 *      Writes the given opcode string into the output buffer.
 *
 * RETURNS
 *      none
 *
 ************************************************************/
 
static void opcode( const char *opcode )
{
    int n = sprintf( output_buffer, "%-*s", dasm_max_opcode_width, opcode );
    output_buffer += n;
}

/***********************************************************
 *
 * FUNCTION
 *      walk_table
 *
 * DESCRIPTION
 *      Disassembles the next instruction in the input stream.
 *      f - file stream to read (pass to calls to next() )
 *      outbuf - pointer to output buffer
 *      addr - address of first input byte for this insn
 *
 * RETURNS
 *      INSN_FOUND if a valid instruction found.
 *      INSN_NOT_FOUND otherwise.
 *
 ************************************************************/

static int walk_table( FILE * f, ADDR * addr, optab_t * optab, UBYTE opc )
{
    UBYTE peek_byte;
    int have_peeked = 0;
    
    if ( optab == NULL )
        return 0;
        
    while ( optab->opcode != NULL )
    {
        /* printf("type:%d  ", optab->type); */
        if ( optab->type == OPTAB_TABLE && optab->opc == opc )
        {
            opc = next( f, addr );
            return walk_table( f, addr, optab->u.table, opc );
        }
        else if ( ( optab->type == OPTAB_INSN && opc == optab->opc )
                    ||
                    ( optab->type == OPTAB_RANGE 
                      && opc >= optab->u.range.min 
                      && opc <= optab->u.range.max )
                    ||
                    ( optab->type == OPTAB_MASK 
                      && ( ( opc & optab->u.mask.mask ) == optab->u.mask.val ) ) )
        {
            opcode( optab->opcode );
            optab->operands( f, addr, opc, optab->xtype );
            return INSN_FOUND;
        }
        else if ( optab->type == OPTAB_MASK2 && opc == optab->opc )
        {
            if ( !have_peeked )
            {
                peek_byte = peek( f );
                have_peeked = 1;
            }
            
            if ( ( peek_byte & optab->u.mask.mask ) == optab->u.mask.val )
            {
                opcode( optab->opcode );
                optab->operands( f, addr, opc, optab->xtype );
                return INSN_FOUND;
            }
        }
        else if ( optab->type == OPTAB_MEMMOD 
                  && ( opc == 0x16 || opc == 0x17 || opc == 0x06 || opc == 0x0A ) )
        {
            if ( !have_peeked )
            {
                peek_byte = peek( f );
                have_peeked = 1;
            }
            
            if ( ( peek_byte & 0x8F ) == optab->opc )
            {
                opcode( optab->opcode );
                optab->operands( f, addr, opc, optab->xtype );
                return INSN_FOUND;
            }        
        }
        
        optab++;    
    }
    
    return INSN_NOT_FOUND;
}

/*****************************************************************************
 *        Public Functions
 *****************************************************************************/

/***********************************************************
 *
 * FUNCTION
 *      operand
 *
 * DESCRIPTION
 *      Writes the given operand string and any arguments
 *      into the output buffer.  The string is processed with
 *      the usual printf() conversions.
 *
 * RETURNS
 *      none
 *
 ************************************************************/
 
void operand( const char *operand, ... )
{
    va_list ap;
    int n;
    
    va_start( ap, operand );
    n = vsprintf( output_buffer, operand, ap );
    va_end( ap );
    
    output_buffer += n;
}

/***********************************************************
 *
 * FUNCTION
 *      dasm_insn
 *
 * DESCRIPTION
 *      Disassembles the next instruction in the input stream.
 *      f - file stream to read (pass to calls to next() )
 *      outbuf - pointer to output buffer
 *      addr - address of first input byte for this insn
 *
 * RETURNS
 *      address of next input byte
 *
 ************************************************************/
 
ADDR dasm_insn( FILE *f, char *outbuf, ADDR addr )
{
    int opc;
    int found = 0;

    /* Store start address in a global for use in xref calls */    
    g_insn_addr = addr;
    
    /* Setup g_output_buffer to point to caller's output buffer */
    output_buffer = outbuf;

    /* Get first opcode byte */
    opc = next( f, &addr );

    /* Now walk table(s) looking for an instruction match */
    found = walk_table( f, &addr, base_optab, opc );
    
    /* If we didn't find a match, indicate this to the output */
    if ( found != INSN_FOUND )
        opcode( "???" );
    
    return addr;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

