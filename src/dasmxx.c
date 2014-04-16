/*****************************************************************************
 *
 * Copyright (C) 2014, Neil Johnson
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
 *****************************************************************************
 *
 * Usage
 *
 * This program makes use of a command file, specified on the command line at
 *  runtime.  This file contains the name of the input file, together with 
 *  various disassembling commands to aid the disassembler make sense of the
 *  memory map, and the different types of data (or code) within different
 *  segments of memory.
 *
 * Command line:
 *
 *      dasmXX listfile
 *
 * where
 *      XX         - target name (78k, 96, etc)
 *      listfile   - is the name of the command list file
 *
 * The command list file contains a list of memory segment definitions, used during
 *  processing to tell the disassembler what the memory at a particular address
 *  is for (unknown, code, or some sort of data).
 *
 * The commands are (where XXXX denotes hexadecimal address):
 *
 * File commands:
 *      fName       input file = `Name'
 *      iName       include file `Name' in place of include command
 *
 * Configuration commands:
 *      tXX         string terminator byte (default = 00)
 *      eXXXX       end of disassembly
 *      rSSSS,EEEE  set cross-reference range, SSSS = start addr, EEEE = end addr
 *
 * Dump commands:
 *      aXXXX       alphanumeric dump
 *      bXXXX       byte dump
 *      sXXXX       string dump
 *      wXXXX       word dump
 *      vXXXX       vector address dump
 *
 * Code disassembly commands:
 *      cXXXX       code disassembly starts at XXXX
 *      pXXXX       procedure start
 *      lXXXX       attach a label to address XXXX
 *      kXXXX       one-line (k)comment for address XXXX
 *      nXXXX       multi-line block comment, ends with line starting '.'
 *
 *  Commands c,b,s,e,w,a,p,l can have a comment string separated from the
 *   address by whitespace (tab or space).  The comment is printed in
 *   the listing.
 *
 *  The labels attached via 'p' and 'l' will be used both in the XREF dump
 *   at the end, and within the disassembly.  For example:
 *
 *       p1234    TestFunc
 *
 *   identifies address 1234 as the entry point of procedure "TestFunc", and
 *   this label will be used in the disassembly, as in:
 *
 *       ljmp      TestFunc
 *
 *   rather than the less-readable:
 *
 *       ljmp      1234
 *
 *  The 'l' command is similar, and can be used to identify branch or jump
 *   targets (loops, tests, etc) and data (tables, strings, etc).
 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "dasmxx.h"

/*****************************************************************************
 *        Data Types, Macros, Constants
 *****************************************************************************/

/* Comment list type */
struct comment {
    int             ref;
    char            *text;
    struct comment  *next;
};

/* Dump format list type */
struct fmt {
    int             mode;
    ADDR             addr;
    char            *name;
    struct fmt      *n;
};

/* Set various physical limits */
#define BYTES_PER_LINE  16
#define NOTE_BUF_SIZE   1024
#define COL_LINECOMMENT 60

/*****************************************************************************
 *        Global Data
 *****************************************************************************/

struct comment  *linecmt    = NULL;
struct comment  *blockcmt   = NULL;
struct fmt      *list       = NULL;

int             string_terminator = '\0';

/* List of display modes.  defines must match entry position. */
static char datchars[] = "cbsewapv";
#define CODE            0
#define BYTES           1
#define STRINGS         2
#define END             3
#define WORDS           4
#define CHARS           5
#define PROCS           6
#define VECTORS         7





static char *inputfile = NULL;

static UBYTE *insn_byte_buffer = NULL;
static UBYTE  insn_byte_idx    = 0;


/*****************************************************************************
 *        Private Functions
 *****************************************************************************/

/***********************************************************
 *
 * FUNCTION
 *      addcomment
 *
 * DESCRIPTION
 *      Adds a single-line comment to the given list.
 *
 * RETURNS
 *      void
 *
 ************************************************************/

static void addcomment( struct comment **list, ADDR ref, char *text )
{
	struct comment  *p;
	struct comment  *q;

	if ( !xref_inrange( ref ) )
		return;

	p = *list;
	q = NULL;

	/* Find entry in specified comment list */

	while ( p != NULL && ref > p->ref )
	{
		q = p;
		p = p->next;
	}

	if ( p != NULL && ref == p->ref)  /* new addr for ref */
	{
		if ( p->text )
			error( "Error: multiple comments for same address. Aborting\n" );
		else
			p->text = dupstr( text );
	}
	else /* insert */
	{
		if ( q == NULL )
		{
			q = zalloc( sizeof( struct comment ) );
			*list = q;
		}
		else
		{
			q->next = zalloc( sizeof( struct comment ) );
			q = q->next;
		}
		q->next  = p;
		q->ref   = ref;
		q->text  = dupstr( text );
	}
}

/***********************************************************
 *
 * FUNCTION
 *      printcomment
 *
 * DESCRIPTION
 *      Searches the given comment list for an entry at
 *       the given address.  If found, prints it.
 *
 * RETURNS
 *      0 if no comment, else 1
 *
 ************************************************************/

static int printcomment( struct comment *list, ADDR ref, int padding )
{
	int i;
	char *p;
	struct comment *plist = list;

	while( plist )
	{
		if ( plist->ref == ref )
		{
			for ( ; padding > 0; padding-- )
				putchar( ' ' );

			printf( "; " );
			for ( p = plist->text; *p; p++ )
			{
				if ( *p == '\n' )
					printf( "\n; " );
				else
					putchar( *p );
			}

			if ( list == blockcmt )
				putchar( '\n' );

			return 1;
		}
		plist = plist->next;
	}    

	return 0;
}

/***********************************************************
 *
 * FUNCTION
 *      commentexists
 *
 * DESCRIPTION
 *      Searches the given comment list for an entry at
 *       the given address.  If found, returns 1, else 0.
 *
 * RETURNS
 *      0 if no comment, else 1
 *
 ************************************************************/

static int commentexists( struct comment *list, ADDR ref )
{
	while( list )
	{
		if ( list->ref == ref )
			return 1;
		list = list->next;
	}    

	return 0;
}

/***********************************************************
 *
 * FUNCTION
 *      emitaddr
 *
 * DESCRIPTION
 *      Emits an address, and optionally a label if it is
 *       a jump target.
 *
 * RETURNS
 *      number of chars emitted
 *
 ************************************************************/

static int emitaddr( ADDR addr )
{
	char * label = xref_findaddrlabel( addr );

   if ( label )
      printf( "%s:\n", label );

   return printf( "    " FORMAT_ADDR ":    ", addr);
}

/***********************************************************
 *
 * FUNCTION
 *      addlist
 *
 * DESCRIPTION
 *      adds an item to the dump formating list
 *
 * RETURNS
 *      void
 *
 ************************************************************/

static void addlist( ADDR addr, int mode, char *name )
{
	struct fmt *p = list, *q = NULL;

	/* scan through address-ordered list to find right place to insert */
	while ( p != NULL && p->addr < addr )
	{
		q = p;
		p = p->n;
	}

	if ( q == NULL )
	{
		/* Insert at head of list */
		q = zalloc( sizeof( struct fmt ) );
		list = q;
	}
	else
	{
		/* Insert within list */
		q->n = zalloc( sizeof( struct fmt ) );
		q = q->n;
	}

	/* Fill in the blanks */
	q->addr = addr;
	q->mode = mode;
	q->n    = p;
	if ( name != NULL )
		q->name = dupstr( name );
	else
		q->name = NULL;
}

/***********************************************************
 *
 * FUNCTION
 *      readlist
 *
 * DESCRIPTION
 *      reads and parses the listfile
 *
 * RETURNS
 *      none
 *
 ************************************************************/

#define LINE_BUF_LEN		( 256 )
#define SKIP_SPACE(M_p)	do {\
									while(*M_p && isspace(*M_p))\
										M_p++;\
								} while(0)

static void readlist( char *name )
{
	FILE *f;
	char buf[LINE_BUF_LEN + 1], *pbuf, *q;
	ADDR addr;
	int cmd;
	int n;
	char notebuf[NOTE_BUF_SIZE + 1];
	int notelength;
	enum {
		LINE_CMD,
		LINE_NOTE
	} linemode = LINE_CMD;

	
	f = fopen( name, "r" );
	if ( !f )
		error( "Failed to open list command file \"%s\".\n", name );

	list = NULL;

	/* Process each line of list file */
	while ( ( pbuf = fgets( buf, LINE_BUF_LEN, f ) ) != NULL )
	{
		if ( linemode == LINE_CMD )
		{
			/* strip leading whitespace */
			SKIP_SPACE(pbuf);
			
			/* Skip comment and blank lines */
			if ( *pbuf == '#' || *pbuf == '\n' )
				continue;
			
			/* Remove trailing newline char */
			if ( q = strchr( pbuf, '\n' ) )
				*q = '\0';

			/* Peel off command code, then do something about it */
			cmd = *pbuf++;
			switch ( cmd )
			{
			case 'a': /* alphanumeric dump        */
			case 'b': /* byte dump                */
			case 'c': /* start of code disassebly */
			case 'e': /* end of processing        */
			case 'p': /* start of procedure       */
			case 's': /* string dump              */
			case 'v': /* vector dump              */
			case 'w': /* word dump                */
				{
					sscanf( pbuf, "%x%n", &addr, &n );
					pbuf += n;

					SKIP_SPACE(pbuf);
						
					/* If user has provided and optional name for this entity then
					 * store it in the xref database.
					 */
					if ( pbuf )
						xref_addxreflabel( addr, pbuf );

					addlist( addr, strchr( datchars, cmd ) - datchars, pbuf );
				}
				break;

			case 'f':  /* inputfile */
				inputfile = dupstr( pbuf );
				break;

			case 'i':   /* include file */
				readlist( pbuf );
				break;

			case 'r':   /* Xref range */
				{
					ADDR ssss,eeee;

					sscanf( pbuf, "%x,%x", &ssss, &eeee );
					xref_setmin( ssss );
					xref_setmax( eeee );
				}
				break;

			case 't':   /* String terminator byte */
				sscanf( pbuf, "%x", &string_terminator );
				break;

			case 'l':   /* Define xref label */
				{
					sscanf( pbuf, "%x%n", &addr, &n );
					pbuf += n;
					
					SKIP_SPACE(pbuf);
					if ( *pbuf )
						xref_addxreflabel( addr, pbuf );
				}
				break;

			case 'k':   /* Single-line (k)comment */
				{
					sscanf( pbuf, "%x%n", &addr, &n );
					pbuf += n;
					
					SKIP_SPACE(pbuf);
					if ( *pbuf )
						addcomment( &linecmt, addr, pbuf );
				}
				break;

			case 'n':   /* Multiple-line note */
				{
					sscanf( pbuf, "%x%n", &addr, &n );
					pbuf += n;
					
					/* Initialise the note buffer and switch to LINE_NOTE mode. */
					notelength = 0;
					notebuf[0] = '\0';
					linemode = LINE_NOTE;

					SKIP_SPACE(pbuf);
					if ( *pbuf )
					{
						strcpy( notebuf, pbuf );
						notelength = strlen( pbuf );
					}
				}
				break;
			}   /* switch */
		}
		else    /* LINE_NOTE */
		{
			/* Note mode is terminated by a line starting with '.' */
			if ( *pbuf == '.' )
			{
				linemode = LINE_CMD;
				addcomment( &blockcmt, addr, notebuf );
			}
			else
			{
				size_t len = MIN( strlen( pbuf ), NOTE_BUF_SIZE - notelength );
				strncat( notebuf, pbuf, len );
			}
		}
	}

	fclose( f );
}

/***********************************************************
 *
 * FUNCTION
 *      usage
 *
 * DESCRIPTION
 *      prints out usage info for user.
 *
 * RETURNS
 *      nothing
 *
 ************************************************************/

static void usage( void )
{
	printf( "%s -- %s Disassembler --\n"
	        "Usage:\n  %s listfile [startaddr]\n",
			  dasm_name, dasm_description, dasm_name );
	exit(EXIT_FAILURE);
}

/*****************************************************************************
 *        Public Functions
 *****************************************************************************/
 
/***********************************************************
 *
 * FUNCTION
 *      error
 *
 * DESCRIPTION
 *      prints out error message and exits.
 *
 * RETURNS
 *      nothing
 *
 ************************************************************/

void error( char *fmt, ... )
{
	va_list ap;

	va_start( ap, fmt ); 
	fprintf( stderr, "%s :: Error :: ", dasm_name );
   vfprintf( stderr, fmt, ap );
	fprintf( stderr, "\n" );
	va_end( ap );

	exit(EXIT_FAILURE);
}

/***********************************************************
 *
 * FUNCTION
 *      zalloc
 *
 * DESCRIPTION
 *      Allocate and zero a block of memory.
 *
 * RETURNS
 *      Pointer to allocated block.
 *
 ************************************************************/

void *zalloc( size_t n )
{
	void *p = calloc( 1, n );
	
	if ( !p )
		error( "out of memory" );
		
	return p;
}

/***********************************************************
 *
 * FUNCTION
 *      dupstr
 *
 * DESCRIPTION
 *      Duplicate a string.  Abort if out of memory.
 *
 * RETURNS
 *      Pointer to new copy of string.
 *
 ************************************************************/

char * dupstr( const char *s )
{
	char *p = strdup( s );
	if ( !p )
		error( "out of memory" );
		
	return p;
}

/***********************************************************
 *
 * FUNCTION
 *      next
 *
 * DESCRIPTION
 *      Gets the next byte from the file stream.  If EOF then abort.
 *
 * RETURNS
 *      next byte in fp
 *
 ************************************************************/

UBYTE next( FILE* fp, ADDR *addr )
{
	int c;
	
	c = fgetc( fp );
	if ( c == EOF )
		error( "run out of input file" );
		
	if ( insn_byte_idx < dasm_max_insn_length )
		insn_byte_buffer[insn_byte_idx++] = (UBYTE)c;
	
	(*addr)++;
	return (UBYTE)c;
}

/***********************************************************
 *
 * FUNCTION
 *      peek
 *
 * DESCRIPTION
 *      Gets the next byte from the file stream but does not
 *       increment the stream pointer.  If EOF then abort.
 *
 * RETURNS
 *      next byte in fp
 *
 ************************************************************/

UBYTE peek( FILE *fp )
{
	int c;
	
	c = fgetc( fp );
	if ( c == EOF )
		error( "run out of input file" );
		
	c = ungetc( c, fp );
	if ( c == EOF )
		error( "unable to peek at input file" );
	
	return (UBYTE)c;
}

/***********************************************************
 *
 * FUNCTION
 *      main
 *
 * DESCRIPTION
 *      called at startup.
 *
 * RETURNS
 *      nothing
 *
 ************************************************************/

int main(int argc, char **argv)
{
	 FILE *f;
	 long filelength;
	 int c;
	 ADDR addr, startaddr, lineaddr;
	 int i;
	 int mode;
	 char *name;


    /* Check if enough args */
    if ( argc < 2 )
        usage();
    
    /* Process first arg: listfile */
    readlist( argv[1] );
    
    if ( !inputfile )
        error( "no input file specified" );
    
    printf( "    %s -- %s Disassembler --\n"
            "-----------------------------------------------------------------\n"
            "    Input file       : %s\n"
            "-----------------------------------------------------------------\n\n",
				dasm_name,
				dasm_description,
            inputfile );
    
    f = fopen( inputfile, "rb" );
	 
	 if ( !f )
	    error( "failed to open input file" );
	 
	 fseek( f, 0, SEEK_END );
	 filelength = ftell( f );
	 fseek( f, 0, SEEK_SET );
	 
	 printf( "   Processing \"%s\" (%ld bytes)\n", inputfile, filelength );
    
    if ( list )
        addr = list->addr;
	 else
	     error( "empty list file" );
	 
	 printf( "   Disassembly start address: 0x%04X\n", addr );
	 printf( "   String terminator: 0x%02x\n", string_terminator );
    
    mode = list->mode;
    name = list->name;
    list = list->n;
	 
	 insn_byte_buffer = zalloc( dasm_max_insn_length );
	 insn_byte_idx = 0;
    
    while ( !feof( f ) && list )
    {
        if ( addr >= list->addr )
        {
            if ( mode != list->mode ) putchar ( '\n' );
            mode = list->mode;
            name = list->name;
            list = list->n;
        }

        if ( mode == CODE )
        {
            /*****************************************************************
             *            c - CODE
             *****************************************************************/
            int column, i;
				
            printcomment( blockcmt, addr, 0 );
            
            column = emitaddr( addr);
            lineaddr = addr;
				insn_byte_idx = 0;
            
				{
					char insnbuf[256];
					
				   addr = dasm_insn( f, insnbuf, addr );
					
					for ( i = 0; i < dasm_max_insn_length; i++ )
						if ( i < insn_byte_idx )
							printf( "%02X ", insn_byte_buffer[i] );
						else
							printf( "   " );
							
					printf( "   " );
					
					i = printf("%s", insnbuf );
				}
				column += i;
            
            printcomment( linecmt, lineaddr, COL_LINECOMMENT - column );
            putchar( '\n' );
        }
        else if ( mode == BYTES )
        {
            /*****************************************************************
             *            b - BYTES
             *****************************************************************/
            
            unsigned char buf[BYTES_PER_LINE];
            int p;
            
            putchar( '\n' );
            printcomment( blockcmt, addr, 0 );
            
            i = 0;
            while ( addr < list->addr )
            {
                if ( i == 0 ) 
                {
                    emitaddr( addr );
                    printf( "DB      " );
                }
                
                buf[i] = (unsigned char)next( f, &addr );
                printf( "%02X ", (unsigned char)buf[i] );
                i++;
                if ( i == BYTES_PER_LINE )
                {
                    /* End of a full line */
                    printf( "      " );
                    
                    for ( p = 0; p < BYTES_PER_LINE; p++ )
                        if ( isprint( buf[p] ) )
                            putchar( buf[p] );
                        else
                            putchar( '.' );
                    
                    putchar( '\n' );
                    i = 0;
                }
            }
            if ( i < BYTES_PER_LINE )
            {
                /* Partial line, tricky */
                
                for ( p = i; p < BYTES_PER_LINE; p++ )
                    printf( "   " );
                
                printf( "      " );

                for ( p = 0; p < i; p++ )
                    if ( isprint( buf[p] ) )
                        putchar( buf[p] );
                    else
                        putchar( '.' );
                
                putchar( '\n' );
            }
            
            mode = list->mode;
            if ( mode == CODE || mode == PROCS )
                putchar( '\n' );
            name = list->name;
            list = list->n;
        }
        else if ( mode == STRINGS )
        {
            /*****************************************************************
             *            s - STRING DATA
             *****************************************************************/
            
            putchar( '\n' );            
            printcomment( blockcmt, addr, 0 );
            
            while ( addr < list->addr )
            {
                emitaddr( addr );
                printf( "DB      '" );
                
                while ( c = next( f, &addr ) )
                {
                    if ( c == string_terminator )
                        break;
                    
                    if ( isprint( c ) )
                        putchar( c );
                    else
                        printf ("\\%02X", (unsigned char) c );
                }
                printf( "'\n" );
            }
            
            mode = list->mode;
            if ( mode == CODE || mode == PROCS )
                putchar( '\n' );
            name = list->name; 
            list = list->n;
        }
        else if ( mode == WORDS )
        {
            /*****************************************************************
             *            w - WORD DATA
             *****************************************************************/
            
            putchar( '\n' );
            printcomment( blockcmt, addr, 0 );
            
            i = 0;
            while ( addr < list->addr )
            {
                if ( ( i & 7 ) == 0 ) 
                {
                    emitaddr( addr );
                    printf( "DW      " );
                }

                c = (unsigned char)next( f, &addr );

                c = c | ((unsigned char)next( f, &addr ) << 8 );

                printf( "%04X ", c );
                xref_addxref( X_TABLE, addr - 2, c );
                
                if ( ( i & 7 ) == 7 )
                    putchar( '\n' );
                i++;
            }
            if ( i & 7 ) 
                putchar( '\n' );

            mode = list->mode;
            if ( mode == CODE || mode == PROCS )
                putchar( '\n' );
            name = list->name;
            list = list->n;
        }
		  else if ( mode == VECTORS )
        {
            /*****************************************************************
             *            v - VECTOR DATA
             *****************************************************************/
            
            putchar( '\n' );
            printcomment( blockcmt, addr, 0 );
            
            i = 0;
            while ( addr < list->addr )
            {
                emitaddr( addr );
                printf( "DW      " );

                c = (unsigned char)next( f, &addr );

                c = c | ((unsigned char)next( f, &addr ) << 8 );

                printf( "%s\n", xref_genwordaddr( NULL, "", c ) );
					 xref_addxref( X_TABLE, addr - 2, c );
                
                i++;
            }

            mode = list->mode;
            if ( mode == CODE || mode == PROCS )
                putchar( '\n' );
            name = list->name;
            list = list->n;
        }
        else if ( mode == CHARS )
        {
            /*****************************************************************
             *            a - CHARS (alphanums)
             *****************************************************************/
            
            putchar( '\n' );
            printcomment( blockcmt, addr, 0 );
            
            i = 0;
            while ( addr < list->addr )
            {
                if ( ( i & 7 ) == 0 )
                {
                    emitaddr( addr );
                    printf( "DB      " );
                }

                c = next( f, &addr );
                
                if ( isprint( c ) )
                    printf( "'%c',", c );
                else
                    printf( "%02X,", (unsigned char)c );

                if ( ( i & 7 ) == 7 ) 
                    putchar( '\n' );
                i++;
            }
            if ( i & 7 ) 
                putchar( '\n' );

            mode = list->mode;
            if ( mode == CODE || mode == PROCS )
                putchar( '\n' );
            name = list->name; 
            list = list->n;
        }
        else if ( mode == END )
        {
            /*****************************************************************
             *            e - END
             *****************************************************************/
            
            list = NULL;
        }
        else if ( mode == PROCS )
        {
            /*****************************************************************
             *            p - PROCS
             *****************************************************************/
            
            if ( !commentexists( blockcmt, addr ) )
            {
                printf( "----------------------------------------------------------------\n"
                        "        Function: %s\n\n", ( name ) ? name : "" );
            }

            mode = CODE;
        }
    } /* while() */
    
    fclose( f );
    printf( "\n\nXREFS :\n\n" );
    xref_dump();

    return EXIT_SUCCESS;
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
