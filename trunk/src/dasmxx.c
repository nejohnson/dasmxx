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
 *      dasmXX listfile [start_addr]
 *
 * where
 *      XX         - target name (78k, 96, etc)
 *      listfile   - is the name of the command list file
 *      start_addr - is the start address (TBD)
 *
 * The command list file contains a list of memory segment definitions, used during
 *  processing to tell the disassembler what the memory at a particular address
 *  is for (unknown, code, or some sort of data).
 *
 * The commands are (where XXXX denotes hexadecimal address):
 *
 *      fName       input file = `Name'
 *      iName       include file `Name' in place of include command
 *      cXXXX       code starts at XXXX
 *      bXXXX       byte dump
 *      sXXXX       string dump
 *      eXXXX       end of disassembly
 *      wXXXX       word dump
 *      vXXXX       vector address dump
 *      aXXXX       alphanum dump
 *      pXXXX       procedure start
 *      lXXXX       attach a label to address
 *      tXX         string terminator byte (default = 00)
 *      rFXXXX      set cross-reference range (F=0 start, F=1 end)
 *      kXXXX       one-line (k)comment for address XXXX
 *      nXXXX       multi-line block comment
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
 
struct comment {
    int             ref;
    char            *text;
    struct comment  *next;
};

struct fmt {
    int             mode;
    int             addr;
    char            *name;
    struct fmt      *n;
};




#define BYTES_PER_LINE  16
#define NOTE_BUF_SIZE   1024
#define COL_LINECOMMENT 60

/*****************************************************************************
 *        Global Data
 *****************************************************************************/

struct comment  *linecmt    = NULL;
struct comment  *blockcmt   = NULL;
struct fmt      *list;

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





char *inputfile = NULL;





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

static void addcomment( struct comment **list, int ref, char *text )
{
    struct comment  *p;
    struct comment  *q;
    
    if ( !xref_inrange( ref ) )
        return;
    
    p = *list;
    q = NULL;
    
    /* Find entry in specified comment list */
    
    while ( p != NULL && (unsigned)ref > (unsigned)p->ref )
    {
        q = p;
        p = p->next;
    }

    if ( p != NULL && ref == p->ref)  /* new addr for ref */
    {
        if ( p->text )
        {
            error( "Error: multiple comments for same address. Aborting\n" );
        }
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

static int printcomment( struct comment *list, int ref, int padding )
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

static int commentexists( struct comment *list, int ref )
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

static int emitaddr( int addr )
{
	char * label = xref_findaddrlabel( addr );

   if ( label )
      printf( "%s:\n", label );

   return printf( "    %04X:    ", addr);
}

/***********************************************************
 *
 * FUNCTION
 *      addlist
 *
 * DESCRIPTION
 *      adds an item to the list
 *
 * RETURNS
 *      void
 *
 ************************************************************/

static void addlist( int addr, int mode, char *name )
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

static void readlist( char *name )
{
    FILE *f;
    char buf[256], *p, *q;
    int addr;
    int cmd;
    char notebuf[NOTE_BUF_SIZE];
    int notelength;
    enum {
        LINE_CMD,
        LINE_NOTE
    } linemode = LINE_CMD;
        
            
    if ( ( f = fopen( name, "r" ) ) == NULL )
        error( "Failed to open list command file \"%s\".\n", name );
    
    list = NULL;
    
    /* Process each line of list file */
    while ( ( p = fgets( buf, 80, f ) ) != NULL )
    {
        buf[strlen( buf ) - 1] = 0;    /* clear newline char */
        
        if ( linemode == LINE_CMD )
        {
            /* Skip comment lines */
            if ( buf[0] == '#' )
                continue;

            /* skip until we find a data specifier character */        
            for ( q = datchars; *q != 0 && *q != p[0]; q++ ) ;

            if ( *q )
            {
                char *name = NULL;
                int count;

                /* Process data specifier */

                /* - get address */
                sscanf( p + 1, "%x%n", &addr, &count );

                count++;
                if ( buf[count] )
                {
                    while( buf[count] && isspace( buf[count] ) )
                        count++;
                    name = &buf[count];
                }

                addlist( addr, q - datchars, name );

                if ( name )
                    xref_addxreflabel( addr, name );
            }
            else
            {
                /* Some other listfile command */

                switch( p[0] )
                {
                    case 'f':  /* inputfile */
                        inputfile = zalloc( strlen( p ) );
                        strcpy( inputfile, p + 1 );
                        break;
								
						 case 'i':   /* include file */
						     readlist( &p[1] );
						     break;

                    case 'r':   /* Xref range */
						  {
							  unsigned int rng;
							  
							  sscanf( p + 2, "%x", &rng );
                        if ( p[1] == '0' )
                            xref_setmin(rng);
                        else
                            xref_setmax( rng );
                        break;
								}

                    case 't':   /* String terminator byte */
                        sscanf( p + 1, "%x", &string_terminator );
                        break;

                    case 'l':   /* Define xref label */
                        {
                            char *label = zalloc( strlen( p ) );

                            sscanf( p + 1, "%x %s", &addr, label );
                            xref_addxreflabel( addr, label );
                            free( label );
                        }
                        break;

                    case 'k':   /* Single-line (k)comment */
                        {
                            int count;

                            sscanf( p + 1, "%x%n", &addr, &count );
                            count++;
                            while ( isspace( p[count] ) )
                                count++;
                            if ( p[count] )
                                addcomment( &linecmt, addr, &p[count] );
                        }
                        break;

                    case 'n':   /* Multiple-line note */
                        {
                            int count;

                            sscanf( p + 1, "%x%n", &addr, &count );
                            count++;
                            notelength = 0;
                            linemode = LINE_NOTE;
                            notebuf[0] = '\0';

                            while( isspace( p[count] ) )
                                count++;
                            if ( p[count] )
                            {
                                strcpy( notebuf, &p[count] );
                                notelength = strlen( &p[count] );
                            }
                        }
                        break;
                }   /* switch */
            }   /* else (*p) */
        }
        else    /* LINE_NOTE */
        {
            if ( p[0] == '.' )
            {
                linemode = LINE_CMD;
                addcomment( &blockcmt, addr, notebuf );
            }
            else
            {
                strcat( notebuf, "\n" );
                notelength++;
                if ( notelength + strlen( p ) < NOTE_BUF_SIZE )
                    strcat( notebuf, p );
                else if ( ( NOTE_BUF_SIZE - notelength ) > 0 )
                    strncat( notebuf, p, ( NOTE_BUF_SIZE - notelength ) );
            }
        }
    }
    
    fclose(f);
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
	
	if ( p == NULL )
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

 int next( FILE* fp, int *addr )
{
	int c;
	
	c = getc( fp );
	if ( feof( fp ) )
		error( "run out of input file" );
		
	(*addr)++;
	return c;
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
	 int addr, startaddr, lineaddr;
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
    
    /* If third arg (start addr) get it and convert to address */
    if ( argc == 3 )
    {
        startaddr = strtol( argv[2], (char **)NULL, 0 );
        fseek( f, (unsigned long)startaddr - list->addr, SEEK_SET );
        while ( startaddr > list->addr )
            list = list->n;
        addr = startaddr;
    }
    else if ( list )
        addr = list->addr;
	 else
	     error( "empty list file" );
	 
	 printf( "   Start address: 0x%04X\n", addr );
	 printf( "   String terminator: 0x%02x\n", string_terminator );
    
    mode = list->mode;
    name = list->name;
    list = list->n;
    
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
            
				{
					char insnbuf[256];
				   addr = dasm_insn( f, insnbuf, addr );
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
