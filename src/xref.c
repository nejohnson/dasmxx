/*****************************************************************************
 *
 * Copyright (C) 2014-2016, Neil Johnson
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

/*****************************************************************************
 *        Data Types
 *****************************************************************************/

struct addrlist {
    ADDR           addr;
    XREF_TYPE       type;
    struct addrlist *n;
};

struct xref {
    struct xref     *n;
    ADDR            ref;
    char            *label;
    struct addrlist *list;
};

/*****************************************************************************
 *        Global Data
 *****************************************************************************/

struct xref *xref = NULL;

/*****************************************************************************
 *        Public Functions
 *****************************************************************************/
 
/***********************************************************
 *
 * FUNCTION
 *      xref_addxref
 *
 * DESCRIPTION
 *      Adds the given xref to the xref list.
 *
 * RETURNS
 *      void
 *
 ************************************************************/

 void xref_addxref( int type, ADDR addr, ADDR ref )
{
    struct xref     *p;
    struct xref     *q;
    struct addrlist *new;
    
    if ( type == X_NONE )
        return;
    
    /* Create new address reference entry */
    new = zalloc( sizeof( struct addrlist ) );
    
    new->addr = addr;
    new->type = type;

    p = xref;
    q = NULL;
    
    /* Find entry in list for xref */
    
    while ( p != NULL && ref > p->ref )
    {
        q = p;
        p = p->n;
    }

    if ( p != NULL && ref == p->ref)  /* new addr for ref */
    {
        new->n  = p->list;
        p->list = new;
    }
    else /* insert */
    {
        new->n = NULL;
        
        if ( q == NULL )
        {
            q = zalloc( sizeof( struct xref ) );
            xref = q;
        }
        else
        {
            q->n = zalloc( sizeof( struct xref ) );
            q = q->n;
        }
        q->n    = p;
        q->list = new;
        q->label= NULL;
        q->ref  = ref;
    }
}

/***********************************************************
 *
 * FUNCTION
 *      xref_addxreflabel
 *
 * DESCRIPTION
 *      Adds the given xref to the xref list with the given
 *       label.
 *      Must make a copy of label string.
 *
 * RETURNS
 *      void
 *
 ************************************************************/

void xref_addxreflabel( ADDR ref, char *label )
{
    struct xref     *p;
    struct xref     *q;
    struct addrlist *new;
    
    p = xref;
    q = NULL;
    
    /* Find entry in list for xref */
    
    while ( p != NULL && ref > p->ref )
    {
        q = p;
        p = p->n;
    }

    if ( p != NULL && ref == p->ref)  /* new label for ref */
    {
        if ( p->label )
            if ( strncmp( p->label, GEN_LABEL_PREFIX, strlen( GEN_LABEL_PREFIX ) ) )
                error( "multiple labels for same address (0x%X) (was: %s, new:%s)", ref, p->label, label );
            else
                free( p->label );
        
        p->label = dupstr( label );
    }
    else /* insert */
    {
        if ( q == NULL )
        {
            q = zalloc( sizeof( struct xref ) );
            xref = q;
        }
        else
        {
            q->n = zalloc( sizeof( struct xref ) );
            q = q->n;
        }
        q->n     = p;
        q->ref   = ref;
        q->list  = NULL;
        q->label = dupstr( label );
    }
}

/***********************************************************
 *
 * FUNCTION
 *      xref_findaddrlabel
 *
 * DESCRIPTION
 *      Searches the xref table for the given address and returns
 *      pointer to label if found.
 *
 * RETURNS
 *      Pointer to label if found, else NULL.
 *
 ************************************************************/

char * xref_findaddrlabel( ADDR addr )
{
    struct xref *p;
    
    for ( p = xref; p != NULL; p = p->n )
        if ( p->ref == addr && p->label != NULL )
            return p->label;

    return NULL;
}

/***********************************************************
 *
 * FUNCTION
 *      xref_genwordaddr
 *
 * DESCRIPTION
 *      Generates a word address, either as hex or, if in
 *       the xref list and is labelled, then the label.
 *
 * RETURNS
 *      ptr to allocated string buffer
 *
 ************************************************************/

char * xref_genwordaddr( char * buf, const char * format, ADDR addr )
{
    char * label = xref_findaddrlabel( addr );
	 
    if ( label )
        return label;
    
    /* Either xref not found or not labelled */
 
    if ( buf )
        sprintf( buf, format, addr );
    else
    {
        char local[256];
        sprintf( local, format, addr );
        buf = dupstr(local);	
    }
    
    return buf;
}

/***********************************************************
 *
 * FUNCTION
 *      xref_dump
 *
 * DESCRIPTION
 *      Dumps cross-ref table to screen.
 *
 * RETURNS
 *      void
 *
 ************************************************************/

void xref_dump( void )
{
    struct xref *p;
    struct addrlist *q;

    printf( "\n\nXREFS :\n\n---------------------------\n" );
    for ( p = xref; p != NULL; p = p->n )
    {
	int i = 0;

	if ( !p->list )
	    continue;

	for (q = p->list; q != NULL; q = q->n )
	{
	    if ( i++ == 0 )
	        printf( FORMAT_ADDR ": ", p->ref );
	    else
	        printf( "      " );

	    switch( q->type )
	    {
		case X_JMP    : printf( "Jump   @ " ); break;
		case X_CALL   : printf( "Call   @ " ); break;
		case X_IMM    : printf( "Imm    @ " ); break;
		case X_TABLE  : printf( "Table  @ " ); break;
		case X_DIRECT : printf( "Direct @ " ); break;
		case X_DATA   : printf( "Data   @ " ); break;
		case X_PTR    : printf( "Ptr    @ " ); break;
		case X_REG    : printf( "Reg    @ " ); break;
		case X_IO     : printf( "IO     @ " ); break;
		default:
		    printf( "\nILLEGAL XREF TYPE %d, addr=" FORMAT_ADDR ". Aborting..\n",
		    q->type, q->addr );
		    return;
	    }
	    printf( FORMAT_ADDR, q->addr );
	    if ( p->label && i == 1 )
	        printf( "   (%s)", p->label );
	    putchar( '\n' );
	}
	putchar( '\n' );
    }
    puts( "---------------------------\n" );
}
 
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
