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
 *      dasmXX [options] listfile
 *
 * Where
 *      XX         - target name (78k, 96, etc)
 *      listfile   - is the name of the command list file
 *
 * Supported command line options are:
 *      -h         - print helpful usage information
 *      -x         - generate cross-reference list at end of disassembly
 *      -a         - generate assembler source output
 *      -s         - generate stripped assembler output (forces -a)
 *      -o foo     - write output to file "foo" (default is stdout)
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
 *      q[,N]["title"]  pagination, N lines (default=60), optional title
 *
 * Dump commands:
 *      aXXXX       alphanumeric dump
 *      bXXXX[,N]   byte dump (N bytes, default is 16)
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
 *  Note: both 'l' and 'p' use auto-naming: if no name is given then dasmxx
 *   will generate a name for you: "AL_nnnn" for labels, and "PROC_nnnn" for 
 *     procedures.
 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h> /* for getopt */
#include <ctype.h>

#include "dasmxx.h"

/*****************************************************************************
 *        Data Types, Macros, Constants
 *****************************************************************************/

/* Comment list type */
struct comment {
    int              ref;
    char            *text;
    struct comment  *next;
};

/* Dump format list type */
struct fmt {
    int              mode;
    ADDR             addr;
    unsigned int     bpl; /* bytes per line */
    char            *name;
    struct fmt      *n;
};

struct params {
    const char * listfile;
    const char * inputfile;
    const char * outputfile;
    struct fmt * cmdlist;
    
    int want_xref;
    int want_asm_out;
    int want_stripped;
};

/* Set various physical limits */
#define BYTES_PER_LINE  16
#define NOTE_BUF_SIZE   4096
#define COL_LINECOMMENT 60

#define COMMENT_DELIM        ";"

#define SWAP(a,b)   do { int t = a; a = b; b = t; } while(0)

/*****************************************************************************
 *        Global Data
 *****************************************************************************/

struct comment  *linecmt    = NULL;
struct comment  *blockcmt   = NULL;

int             string_terminator = '\0';

/* List of display modes.  Defines must match entry position. */
static char datchars[] = "cbsewapvm";
#define CODE            0
#define BYTES           1
#define STRINGS         2
#define END             3
#define WORDS           4
#define CHARS           5
#define PROCS           6
#define VECTORS         7
#define BITMAPS         8

/* Global instruction byte buffer */
static UBYTE *insn_byte_buffer = NULL;
static UBYTE  insn_byte_idx    = 0;

/* Pagination Formatting */
static int pagination   = 0;
#define PAGINATION_ALLOWANCE        ( 2 )
#define DEFAULT_LINES_PER_PAGE      ( 60 )
#define MIN_LINES_PER_PAGE          ( 10 )
static const char *page_title = NULL;

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
    struct comment *p = *list;
    struct comment *q = NULL;

    /* Find entry in specified comment list */
    while ( p != NULL && ref > p->ref )
    {
        q = p;
        p = p->next;
    }

    if ( p != NULL && ref == p->ref)  /* new addr for ref */
    {
        if ( p->text )
            error( "Multiple comments for same address ($%04X)", ref );

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
 *      emit_page_header
 *
 * DESCRIPTION
 *      Output page header, comprising a page number and an
 *      optional title.
 */
static void emit_page_header( void )
{
    static int page_no = 1;

    if ( pagination )
    {
        printf( "%s Page %d", COMMENT_DELIM, page_no++ );
        if ( page_title )
            printf( " -- %s", page_title ); 
        printf( "\n\n" );
    }
}

/***********************************************************
 *
 * FUNCTION
 *      newline
 *
 * DESCRIPTION
 *      Output a newline.  Also do pagination if required.
 */
static void newline( void )
{
    static int n = 0;

    putchar( '\n' ); n++;

    if ( pagination && n >= pagination )
    {
        n = 0;
        printf( "\f" );
        emit_page_header();
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

static int printcomment( struct comment *list, ADDR ref, unsigned int padding )
{
    int i;
    char *p;
    struct comment *plist = list;
    
    for ( ; plist; plist = plist->next )
    {
        if ( plist->ref == ref )
        {
            printf( "%*s ", padding, COMMENT_DELIM );
            for ( p = plist->text; *p; p++ )
            {
                if ( *p == '\n' )
                {
                    newline();
                    printf( "%*s ", padding, COMMENT_DELIM );
                }
                else
                    putchar( *p );
            }
            
            if ( list == blockcmt )
                newline();

            return 1;
        }
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
    for ( ; list; list = list->next )
        if ( list->ref == ref )
            return 1;

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

static int emitaddr( ADDR addr, struct params *params )
{
    char * label = xref_findaddrlabel( addr );

    if ( label )
    {
        printf( "%s:", label );
        newline();
    }

    if ( !params->want_stripped )
        return printf( "%c   " FORMAT_ADDR ":    ", 
            params->want_asm_out ? ';' : ' ',
            addr );
    else
        return 0;
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

static void addlist( struct fmt **list, ADDR addr, int mode, unsigned int bytes_per_line, char *name )
{
    struct fmt *p = *list, *q = NULL;

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
        *list = q;
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
    q->bpl  = bytes_per_line;
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

#define LINE_BUF_LEN        ( 256 )
#define SKIP_SPACE(M_p)    do {\
                               while(*M_p && isspace(*M_p))\
                                   M_p++;\
                           } while(0)

static void readlist( const char *listfile, struct params *params )
{
    FILE *f;
    char buf[LINE_BUF_LEN + 1], *pbuf, *q;
    ADDR addr;
    int cmd;
    int n;
    char notebuf[NOTE_BUF_SIZE + 1];
    int notelength;
    unsigned int lineno = 0;
   
    enum {
        LINE_CMD,
        LINE_NOTE
    } linemode = LINE_CMD;
    
    if ( !listfile )
        error( "No listfile specifed" );
        
    f = fopen( listfile, "r" );
    if ( !f )
        error( "Failed to open list command file \"%s\"", listfile );

    /* Process each line of list file */
    while ( lineno++, ( pbuf = fgets( buf, LINE_BUF_LEN, f ) ) != NULL )
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
            cmd = tolower( cmd ); /* Be case-agnostic */
            switch ( cmd )
            {
            case 0: break; /* Catch blank line at end of file */

            case 'a': /* alphanumeric character dump */
            case 'b': /* byte dump                   */
            case 'c': /* start of code disassembly   */
            case 'e': /* end of processing           */
            case 'p': /* start of procedure          */
            case 's': /* string dump                 */
            case 'v': /* vector dump                 */
            case 'w': /* word dump                   */
            case 'm': /* bitmap dump                 */
                {
                    unsigned int cmd_idx = strchr( datchars, cmd ) - datchars;
                    unsigned bytes_per_line = BYTES_PER_LINE;
                    sscanf( pbuf, "%x%n", &addr, &n );
                    pbuf += n;
                    
                    if ( *pbuf == ',' )
                    {
                        unsigned int count;
                        
                        if ( cmd != 'b' )
                            error( "%s(%u) :: Byte count not supported for command '%c'", listfile, lineno, cmd );
                        
                        pbuf++;
                        sscanf( pbuf, "%u%n", &count, &n );
                        pbuf += n;
                        
                        if ( count > BYTES_PER_LINE )
                            error( "%s(%u) :: Too many bytes per line (limit is %d)", listfile, lineno, BYTES_PER_LINE );
                            
                        bytes_per_line = count;
                    }

                    SKIP_SPACE(pbuf);
                        
                    /* If user has provided an optional name for this entity then
                     * store it in the xref database.
                     */
                    if ( !*pbuf )
                    {
                        static struct {
                            char *pfx;
                            unsigned int num;
                        } tbl[] = {
                            { "CL",     1 },
                            { "BDATA",  1 }, 
                            { "STRING", 1 },
                            { NULL,     0 }, /* END */
                            { "WDATA",  1 },
                            { "CDATA",  1 },
                            { "PROC",   1 },
                            { "VCTR",   1 },
                            { "BMAP",   1 }
                        };

                        if ( tbl[cmd_idx].pfx )
                            sprintf( pbuf, GEN_LABEL_PREFIX "%s_%04d", tbl[cmd_idx].pfx, tbl[cmd_idx].num++ );
                    }
                    
                    /* Add a cross-ref entry for everything except an end entry */
                    if ( cmd != 'e' )
                        xref_addxreflabel( addr, pbuf );

                    addlist( &(params->cmdlist), 
                                addr, 
                                cmd_idx, 
                                bytes_per_line,
                                pbuf );
                }
                break;

            case 'f':  /* inputfile */
                if ( params->inputfile )
                    error( "%s(%u) :: Multiple input files specified", listfile, lineno );
                params->inputfile = (const char *)dupstr( pbuf );
                break;

            case 'i':   /* include file */
                readlist( pbuf, params );
                break;

            case 'r':   /* Xref range */
                {
                    /* Deprecated */
                }
                break;

            case 't':   /* String terminator byte */
                sscanf( pbuf, "%x", &string_terminator );
                break;

           case 'l':   /* Define xref code label */
           case 'd':   /* Define xref data label */
                {
                    sscanf( pbuf, "%x%n", &addr, &n );
                    pbuf += n;
                    
                    SKIP_SPACE(pbuf);
                    
                    if ( !*pbuf )
                    {
                        static unsigned int auto_label = 1;
                   
                        sprintf( pbuf, "AL_%04d", auto_label++ );
                    }
                    
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

            case 'q':   /* Pagination */
                {
                    pagination = DEFAULT_LINES_PER_PAGE;

                    SKIP_SPACE(pbuf);
                    if ( *pbuf == ',' )
                    {
                        pbuf++;
                        sscanf( pbuf, "%d%n", &pagination, &n );
                        pbuf += n;
                    }

                    if ( pagination < MIN_LINES_PER_PAGE )
                        error( "%s(%u) :: Must be at least %d lines per page\n", 
                                listfile, lineno, MIN_LINES_PER_PAGE );

                    pagination -= PAGINATION_ALLOWANCE;

                    SKIP_SPACE(pbuf);
                    if ( *pbuf == '"' )
                    {
                        char title[LINE_BUF_LEN];
                        strcpy( title, pbuf+1 );
                        title[strlen(title)-1] = '\0';
                        page_title = strdup( title );
                    }
                    else if ( params->inputfile )
                    {
                        page_title = params->inputfile;
                    }
                }
                break;

            default: /* Unknown command */
                {
                    if ( isprint( cmd ) )
                        error( "%s(%u) :: Unknown command code '%d'\n", listfile, lineno, cmd );
                    else
                        error( "%s :: Illegal character in command file - is this a binary file?\n", listfile );
                }
                break;
            }   /* switch */
        }
        else    /* is LINE_NOTE */
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
            "Usage:\n"
            "  %s [options] listfile\n"
            "\n"
            "  options:\n"
            "     -h        print helpful usage information\n"
            "     -x        with cross-reference list\n"
            "     -a        output in assembler format\n"
            "     -s        stripped assembler output (forces -a)\n"
            "     -o foo    write output to `foo' (stdout is default)\n",
            dasm_name, dasm_description, dasm_name );
    exit(EXIT_FAILURE);
}

/***********************************************************
 *
 * FUNCTION
 *      run_disasm
 *
 * DESCRIPTION
 *      Run a complete disassembly pass on the input.
 *
 * RETURNS
 *      nothing
 *
 ************************************************************/
 
static void run_disasm( struct params params )
{ 
    const char *inputfile = params.inputfile;
    struct fmt *clist     = params.cmdlist;
    FILE *f;
    long  filelength;
    ADDR  addr;
    int   mode;
    unsigned int bpl;
    char *name;
    
    f = fopen( inputfile, "rb" );
    if ( !f )
        error( "Failed to open input file" );
        
    fseek( f, 0, SEEK_END );
    filelength = ftell( f );
    fseek( f, 0, SEEK_SET );
    
    addr  = clist->addr;
    mode  = clist->mode;
    name  = clist->name;
    bpl   = clist->bpl;
    clist = clist->n;
    
    printf( "%s   Processing \"%s\" (%ld bytes)", COMMENT_DELIM, inputfile, filelength ); newline();
    printf( "%s   Disassembly start address: 0x%04X", COMMENT_DELIM, addr );              newline();
    printf( "%s   String terminator: 0x%02x", COMMENT_DELIM, string_terminator );         newline();
    newline();

    while ( !feof( f ) && clist )
    {
        if ( addr >= clist->addr )
        {
            if ( mode != clist->mode )
                newline();
            mode  = clist->mode;
            name  = clist->name;
            bpl   = clist->bpl;
            clist = clist->n;
        }
        
        if ( !clist )
            break;

        if ( mode == CODE )
        {
            /*****************************************************************
            *            c - CODE
            *****************************************************************/
            int column, i;
            ADDR lineaddr;
            char insnbuf[256];

            printcomment( blockcmt, addr, 0 );

            column = emitaddr( addr, &params );
            lineaddr = addr;
            insn_byte_idx = 0;

            addr = dasm_insn( f, insnbuf, addr );

            if ( !params.want_stripped )
            {
                for ( i = 0; i < dasm_max_insn_length; i++ )
                    if ( i < insn_byte_idx )
                        printf( "%02X ", insn_byte_buffer[i] );
                    else
                        printf( "   " );

                if ( params.want_asm_out )
                    printf( "\n" );
            }
            
            i = printf( "   %s", insnbuf );
            column += i - 3;

            printcomment( linecmt, lineaddr, COL_LINECOMMENT - column );
            newline();
        }
        else if ( mode == BYTES )
        {
            /*****************************************************************
            *            b - BYTES
            *****************************************************************/

            unsigned char buf[BYTES_PER_LINE];
            int p, i = 0;

            newline();
            printcomment( blockcmt, addr, 0 );

            while ( addr < clist->addr )
            {
                if ( i == 0 ) 
                {
                    emitaddr( addr, &params );
                    if ( params.want_asm_out )
                        printf( params.want_stripped ? "   " : "\n   " );
                    printf( "DB      " );
                }

                buf[i] = (unsigned char)next( f, &addr );
                printf( "%02X", (unsigned char)buf[i] );
                i++;
                if ( i == bpl )
                {
                    /* End of a full line */
                    printf( "      " );
                    if ( params.want_asm_out )
                        printf( "; " );

                    for ( p = 0; p < bpl; p++ )
                        if ( isprint( buf[p] ) )
                            putchar( buf[p] );
                        else
                            putchar( '.' );

                    newline();
                    i = 0;
                }
                else
                    if ( addr < clist->addr ) printf( ", " );
            }
            if ( i < bpl )
            {
                /* Partial line, tricky */

                for ( p = i; p < bpl; p++ )
                    printf( "    " );

                printf( "      " );
                if ( params.want_asm_out )
                    printf( "; " );

                for ( p = 0; p < i; p++ )
                    if ( isprint( buf[p] ) )
                        putchar( buf[p] );
                    else
                        putchar( '.' );

                newline();
            }

            mode = clist->mode;
            if ( mode == CODE || mode == PROCS )
                newline();
            name  = clist->name;
            bpl   = clist->bpl;
            clist = clist->n;
        }
        else if ( mode == STRINGS )
        {
            /*****************************************************************
            *            s - STRING DATA
            *****************************************************************/

            int c;
            
            newline();            
            printcomment( blockcmt, addr, 0 );

            while ( addr < clist->addr )
            {
                emitaddr( addr, &params );
                if ( params.want_asm_out )
                    printf( params.want_stripped ? "   " : "\n   " );
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
                printf( "'" );
                newline();
            }

            mode = clist->mode;
            if ( mode == CODE || mode == PROCS )
                newline();
            name  = clist->name; 
            bpl   = clist->bpl;
            clist = clist->n;
        }
        else if ( mode == WORDS )
        {
            /*****************************************************************
            *            w - WORD DATA
            *****************************************************************/

            int w, b_1st, b_2nd, i = 0;
            
            newline();
            printcomment( blockcmt, addr, 0 );

            while ( addr < clist->addr )
            {
                if ( ( i & 7 ) == 0 ) 
                {
                    emitaddr( addr, &params );
                    if ( params.want_asm_out )
                        printf( params.want_stripped ? "   " : "\n   " );
                    printf( "DW      " );
                }

                b_1st = (unsigned char)next( f, &addr );
                b_2nd = (unsigned char)next( f, &addr );

                if ( dasm_word_msb_first )
                    SWAP( b_1st, b_2nd );

                w = b_1st | ( b_2nd << 8 );

                printf( "%04X", w );
                xref_addxref( X_TABLE, addr - 2, w );

                if ( ( i & 7 ) == 7 )
                    newline();
                else
                    if ( addr < clist->addr ) printf( ", " );
                i++;                
            }
            if ( i & 7 ) 
                newline();

            mode = clist->mode;
            if ( mode == CODE || mode == PROCS )
                newline();
            name  = clist->name; 
            bpl   = clist->bpl;
            clist = clist->n;
        }
        else if ( mode == VECTORS )
        {
            /*****************************************************************
            *            v - VECTOR DATA
            *****************************************************************/

            int v, b_1st, b_2nd, i = 0;
            
            newline();
            printcomment( blockcmt, addr, 0 );

            while ( addr < clist->addr )
            {
                emitaddr( addr, &params );
                if ( params.want_asm_out )
                    printf( params.want_stripped ? "   " : "\n   " );
                printf( "DW      " );

                b_1st = (unsigned char)next( f, &addr );
                b_2nd = (unsigned char)next( f, &addr );

                if ( dasm_word_msb_first )
                    SWAP( b_1st, b_2nd );

                v = b_1st | ( b_2nd << 8 );

                printf( "%s", xref_genwordaddr( NULL, "%04X", v ) ); newline();
                xref_addxref( X_TABLE, addr - 2, v );

                i++;
            }

            mode = clist->mode;
            if ( mode == CODE || mode == PROCS )
                newline();
            name  = clist->name; 
            bpl   = clist->bpl;
            clist = clist->n;
        }
        else if ( mode == CHARS )
        {
            /*****************************************************************
            *            a - CHARS (alphanums)
            *****************************************************************/

            int c, i = 0;
            
            newline();
            printcomment( blockcmt, addr, 0 );

            while ( addr < clist->addr )
            {
                if ( ( i & 7 ) == 0 )
                {
                    emitaddr( addr, &params );
                    if ( params.want_asm_out )
                        printf( params.want_stripped ? "   " : "\n   " );
                    printf( "DB      " );
                }

                c = next( f, &addr );

                if ( isprint( c ) )
                    printf( "'%c'", c );
                else
                    printf( "%02X", (unsigned char)c );

                if ( ( i & 7 ) == 7 ) 
                    newline();
                else
                    if ( addr < clist->addr ) printf( ", " );
                i++;
            }
            if ( i & 7 ) 
                newline();

            mode = clist->mode;
            if ( mode == CODE || mode == PROCS )
                newline();
            name  = clist->name; 
            bpl   = clist->bpl;
            clist = clist->n;
        }
        else if ( mode == END )
        {
            /*****************************************************************
            *            e - END
            *****************************************************************/

            clist = NULL;
        }
        else if ( mode == PROCS )
        {
            /*****************************************************************
            *            p - PROCS
            *****************************************************************/

            if ( !commentexists( blockcmt, addr ) )
            {
                printf( ";----------------------------------------------------------------" );
                newline();
                printf( ";        Function: %s", ( name ) ? name : "" );
                newline(); newline();
            }

            mode = CODE;
        }
        else if ( mode == BITMAPS )
        {
            /*****************************************************************
            *            m - BITMAPS
            *****************************************************************/
            
            newline();
            printcomment( blockcmt, addr, 0 );

            while ( addr < clist->addr )
            {
                UBYTE bitmap;
                UBYTE mask = 0x80;
                
                emitaddr( addr, &params );
                if ( params.want_asm_out )
                    printf( params.want_stripped ? "   " : "\n   " );
                printf( "DB      " );

                bitmap = (UBYTE)next( f, &addr );
                printf( "%02X", bitmap );
                
                printf( "    " );
                if ( params.want_asm_out )
                    printf( ";" );
                    
                printf( " [" );
                for ( ; mask; mask >>= 1 )
                    putchar( bitmap & mask ? '#' : '.' );
                printf( "]" ); newline();
            }

            mode = clist->mode;
            if ( mode == CODE || mode == PROCS )
                newline();
            name  = clist->name; 
            bpl   = clist->bpl;
            clist = clist->n;
        }
    } /* while() */
     
    fclose( f );
}

/***********************************************************
 *
 * FUNCTION
 *      process_args
 *
 * DESCRIPTION
 *      prints out error message and exits.
 *
 * RETURNS
 *      params structure populated by defaults or command line
 *       values.
 *
 ************************************************************/

#define OPTSTRING        "asxho:"

static struct params process_args( int argc, char **argv )
{
    struct params params;
    int opt;
    
    memset( &params, 0, sizeof(params) );
    
    while ((opt = getopt(argc, argv, OPTSTRING)) != -1)
    {
        switch (opt)
        {
        case 's':
            params.want_stripped = 1;
            /* fall through */
        case 'a':
            params.want_asm_out = 1;
            break;
            
        case 'x':
            params.want_xref = 1;
            break;
         
        case 'o':
            params.outputfile = (const char*)dupstr(optarg);
            break;
         
        case 'h':
            usage();
            break;
        
        default: /* '?' */
            error( "Uknown command line option `-%c'.  Use `-h' for help", opt );
        }
    }
    
    params.listfile = argv[optind];

    return params;
}

/***********************************************************
 *
 * FUNCTION
 *      display_banner
 *
 * DESCRIPTION
 *      Shows the program banner.
 *
 * RETURNS
 *      nothing
 *
 ************************************************************/
 
#define SPACER "-----------------------------------------------------------------"

static void display_banner( struct params params )
{
    char *prefix = params.outputfile ? COMMENT_DELIM : "";
    
    printf( "%s   %s -- %s Disassembler --", prefix, dasm_name, dasm_description ); newline();
    printf( "%s" SPACER, prefix ); 
    newline();
    newline();
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
    
    fprintf ( stderr, "%s :: Error :: ", dasm_name );
    vfprintf( stderr, fmt, ap );
    fprintf ( stderr, "\n" );

    va_end( ap );

    exit(EXIT_FAILURE);
}

/***********************************************************
 *
 * FUNCTION
 *      warning
 *
 * DESCRIPTION
 *      prints out warning message.
 *
 * RETURNS
 *      nothing
 *
 ************************************************************/

void warning( char *fmt, ... )
{
    va_list ap;

    va_start( ap, fmt );
    
    fprintf ( stderr, "%s :: Warning :: ", dasm_name );
    vfprintf( stderr, fmt, ap );
    fprintf ( stderr, "\n" );

    va_end( ap );
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
        error( "Out of memory" );
        
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
        error( "Out of memory" );
        
    return p;
}

/***********************************************************
 *
 * FUNCTION
 *      next
 *
 * DESCRIPTION
 *      Reads the next byte from the file stream, stores it in
 *      the instruction buffer, and returns it.
 *      If EOF then abort.
 *
 * RETURNS
 *      next byte in file stream
 *      addr incremented
 *
 ************************************************************/

UBYTE next( FILE* fp, ADDR *addr )
{
    int c;
    
    c = fgetc( fp );
    if ( c == EOF )
        error( "Ran past end of input file" );
        
    if ( insn_byte_idx < dasm_max_insn_length )
        insn_byte_buffer[insn_byte_idx++] = (UBYTE)c;
    
    (*addr)++;
    return (UBYTE)c;
}

/***********************************************************
 *
 * FUNCTION
 *      nextw
 *
 * DESCRIPTION
 *      Gets the next word from the file stream.  
 *      If EOF then abort.
 *      Need to swap the order that bytes are put in the 
 *      byte buffer so that they appear in the right order
 *      in the listing.
 *
 * RETURNS
 *      next word in fp
 *
 ************************************************************/

UWORD nextw( FILE* fp, ADDR *addr )
{
    int lo, hi;
    UWORD w = 0;
    
    lo = fgetc( fp );
    if ( lo == EOF )
        error( "Ran past end of input file" );
        
    hi = fgetc( fp );
    if ( hi == EOF )
        error( "Ran past end of input file" );    
        
    if ( insn_byte_idx < dasm_max_insn_length )
        insn_byte_buffer[insn_byte_idx++] = (UBYTE)hi;
        
    if ( insn_byte_idx < dasm_max_insn_length )
        insn_byte_buffer[insn_byte_idx++] = (UBYTE)lo;
    
    (*addr)++;
    (*addr)++;
    
    if ( dasm_word_msb_first )
        SWAP( lo, hi );
    
    w = ( ( hi & 0xFF ) << 8 ) | ( lo & 0xFF );
    
    return w;
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
        error( "Ran past end of input file" );
        
    c = ungetc( c, fp );
    if ( c == EOF )
        error( "Unable to peek at input file" );
    
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
    struct params params;
    
    params = process_args( argc, argv );

    /* Process first arg: listfile */
    readlist( params.listfile, &params );

    /* Check things are set up ready to run */
    if ( !params.cmdlist )
        error( "Empty list file" );

    if ( !params.inputfile )
        error( "No input file specified" );
        
    /* Prepare then instruction byte buffer */
    insn_byte_buffer = zalloc( dasm_max_insn_length );
    insn_byte_idx = 0;
    
    if ( params.outputfile && !freopen( params.outputfile, "w", stdout ) )
        error( "Failed to open output file \"%s\"", params.outputfile );

    emit_page_header();
    display_banner( params );

    run_disasm( params );

    if ( params.want_xref )
        xref_dump();

    return EXIT_SUCCESS;
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
