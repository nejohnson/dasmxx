/*****************************************************************************
 *
 * Copyright (C) 2019, Neil Johnson
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

DASM_PROFILE( "dasmx86", "Intel x86", 5, 9, 0, 1 )

/*****************************************************************************
 * Private data types, macros, constants.
 *****************************************************************************/

/* Common output formats */
#define FORMAT_NUM_8BIT      "0%02XH"
#define FORMAT_NUM_16BIT     "0%04XH"

/* Construct a 16-bit word out of low and high bytes */
#define MK_WORD(l,h)         ( ((l) & 0xFF) | (((h) & 0xFF) << 8) )

/*****************************************************************************
 * Private data.  Declare as static.
 *****************************************************************************/

static const char * const segregs[4]  = { "ES", "CS", "SS", "DS" };
static const char * const wordregs[8] = { "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI" };
static const char * const byteregs[8] = { "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH" };

static int segpfx = -1;

/*****************************************************************************
 *        Private Functions
 *****************************************************************************/

/******************************************************************************/
/**                            Prefix Functions                              **/
/******************************************************************************/

/*
 * Segment override prefix is of the form
 *        001_SS_110
 */
PREFIX_FUNC(pfx_seg)
{
    int reg = (opc >> 3) & 0x03;
    segpfx = reg + 1;    
}


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



/******************************************************************************/
/**                            Double Operands                               **/
/******************************************************************************/



/******************************************************************************/
/**                            Triple Operands                               **/
/******************************************************************************/



/******************************************************************************/
/** Instruction Decoding Tables                                              **/
/** Note: tables are here as they refer to operand functions defined above.  **/
/******************************************************************************/

optab_t base_optab[] = {

/*----------------------------------------------------------------------------
  DATA TRANSFER
  ----------------------------------------------------------------------------*/
  
  
    INSN( "XLAT",   none, 0xD7, X_NONE )
    INSN( "LAHF",   none, 0x9F, X_NONE )
    INSN( "SAHF",   none, 0x9E, X_NONE )
    INSN( "PUSHF",  none, 0x9C, X_NONE )
    INSN( "POPF",   none, 0x9D, X_NONE )
  
/*----------------------------------------------------------------------------
  ARITHMETIC
  ----------------------------------------------------------------------------*/


    INSN( "AAA",    none, 0x37, X_NONE )
    INSN( "BAA",    none, 0x27, X_NONE )
    INSN( "AAS",    none, 0x3F, X_NONE )
    INSN( "DAS",    none, 0x2F, X_NONE )
    INSN( "CBW",    none, 0x98, X_NONE )
    INSN( "CWD",    none, 0x99, X_NONE )
    
/*----------------------------------------------------------------------------
  LOGIC
  ----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  STRING MANIPULATION
  ----------------------------------------------------------------------------*/
  
/*----------------------------------------------------------------------------
  CONTROL TRANSFER
  ----------------------------------------------------------------------------*/

    INSN( "INT3",  none, 0xCC, X_NONE )
    INSN( "INTO",  none, 0xCE, X_NONE )
    INSN( "IRET",  none, 0xCF, X_NONE )

/*----------------------------------------------------------------------------
  PROCESSOR CONTROL
  ----------------------------------------------------------------------------*/
  
    INSN( "CLC",    none, 0xF8, X_NONE )
    INSN( "CMC",    none, 0xF5, X_NONE )
    INSN( "STC",    none, 0xF9, X_NONE )
    
    INSN( "CLD",    none, 0xFC, X_NONE )
    INSN( "STD",    none, 0xFD, X_NONE )
    
    INSN( "CLI",    none, 0xFA, X_NONE )
    INSN( "STI",    none, 0xFB, X_NONE )
    
    INSN( "HLT",    none, 0xF4, X_NONE )
    INSN( "WAIT",   none, 0x9B, X_NONE )
    
    /* ESC */
    
    INSN( "LOCK",   none, 0xF0, X_NONE )
    
    INSN( "NOP",    none, 0x90, X_NONE )
  
/*----------------------------------------------------------------------------
  SEGMENT OVERRIDE PREFIX
  ----------------------------------------------------------------------------*/

    PREFIX( pfx_seg, 0x26 )
    PREFIX( pfx_seg, 0x2E )
    PREFIX( pfx_seg, 0x36 )
    PREFIX( pfx_seg, 0x3E )

/*----------------------------------------------------------------------------*/

   END
};

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
