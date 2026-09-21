/*****************************************************************************
 *
 * Copyright (C) 2014-2024, Neil Johnson
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
 * The commands are (where XXXX denotes hexadecimal field):
 *
 * File commands:
 *      fName       input file = `Name'
 *      iName       include file `Name' in place of include command
 *      >XXXX       fast forward to offset XXXX from start of file
 *
 * Configuration commands:
 *      tXX         string terminator byte (default = 00)
 *      eXXXX       end of disassembly
 *      q[,N]["title"]  pagination, N lines (default=60), optional title
 *
 * Dump commands:
 *      aXXXX       alphanumeric dump
 *      bXXXX[,N]   byte dump (N bytes, default is 16)
 *      mXXXX       bitmap
 *      sXXXX       string dump
 *      uXXXX       string dump with 16-bit characters (utf-16)
 *      vXXXX       vector address dump
 *      wXXXX       word dump
 *      zXXXX       skip (emits a SKIP with the number of bytes). Source must already be 0-filled.
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
#include <stdint.h>
#include <limits.h>

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
    ADDR             seg; /* segment base in force for this entry ('g' cmd) */
    char            *name;
    struct fmt      *n;
};

struct input_file {
    const char       *path;
    int               has_base;
    ADDR              base;
    struct input_file *next;
};

struct params {
    const char * listfile;
    const char * inputfile;
    struct input_file *inputfiles;
    unsigned int unmapped_input_count;
    const char * outputfile;
    struct fmt * cmdlist;
    
    int want_xref;
    int want_asm_out;
    int want_stripped;
};

struct input_segment {
    ADDR addr;
    unsigned int len;
    UBYTE *data;
};

struct input_image {
    struct input_segment *segments;
    unsigned int count;
    unsigned int cap;
    unsigned int file_count;
    unsigned long source_size;
    const char *format;
};

/* Set various physical limits */
#define BYTES_PER_LINE  16
#define NOTE_BUF_INIT   4096
#define COL_LINECOMMENT 60

#define COMMENT_DELIM        ";"

#define SWAP(a,b)   do { int t = a; a = b; b = t; } while(0)

/*****************************************************************************
 *        Global Data
 *****************************************************************************/

struct comment  *linecmt    = NULL;
struct comment  *blockcmt   = NULL;

int             string_terminator = '\0';
unsigned int    file_offset = 0;

/* Segment base (flat address of offset 0 of the current segment), set by
 * the 'g' command.  Segmented targets (x86) use it to resolve intra-segment
 * branch targets; decoders for flat architectures ignore it.
 */
ADDR            dasm_segment_base = 0;
unsigned int    dasm_cpu_level = 0;
unsigned int    dasm_fpu_level = 0;

/* List of display modes.  Defines must match entry position. */
static char datchars[] = "cbsewapvmuz";
#define CODE            0
#define BYTES           1
#define STRINGS         2
#define END             3
#define WORDS           4
#define CHARS           5
#define PROCS           6
#define VECTORS         7
#define BITMAPS         8
#define WSTRING         9
#define SKIP            10

/* Global instruction byte buffer */
static UBYTE *insn_byte_buffer = NULL;
static UBYTE  insn_byte_idx    = 0;
static ADDR   input_peek_addr  = 0;
static struct input_image input_image = { 0 };

/* Pagination Formatting */
static int pagination   = 0;
#define PAGINATION_ALLOWANCE        ( 2 )
#define DEFAULT_LINES_PER_PAGE      ( 60 )
#define MIN_LINES_PER_PAGE          ( 10 )
static const char *page_title = NULL;

static void load_input_images( struct input_file *files, ADDR default_binary_base_addr );
static void free_input_image( void );

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
            addr / dasm_word_width_bytes);
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

static void addlist( struct fmt **list, ADDR addr, int mode, unsigned int bytes_per_line, ADDR seg, char *name )
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
    q->seg  = seg;
    if ( name != NULL )
        q->name = dupstr( name );
    else
        q->name = NULL;
}

/***********************************************************
 *
 * FUNCTION
 *      add_input_file
 *
 * DESCRIPTION
 *      append an input file to the input file list
 *
 * RETURNS
 *      void
 *
 ************************************************************/

static void add_input_file( struct params *params, const char *path, int has_base, ADDR base )
{
    struct input_file *fnew, *scan;

    if ( !*path )
        error( "Empty input file name" );

    if ( !has_base )
    {
        params->unmapped_input_count++;
        if ( params->unmapped_input_count > 1 )
            error( "Multiple unlocated input files specified; use f@XXXX for mapped ROM images" );
    }

    fnew = (struct input_file *) zalloc( sizeof(*fnew) );
    fnew->path = dupstr( path );
    fnew->has_base = has_base;
    fnew->base = base;

    if ( !params->inputfiles )
        params->inputfiles = fnew;
    else
    {
        for ( scan = params->inputfiles; scan->next; scan = scan->next )
            ;
        scan->next = fnew;
    }

    if ( !params->inputfile )
        params->inputfile = fnew->path;
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
                               while(*M_p && isspace((unsigned char)*M_p))\
                                   M_p++;\
                           } while(0)

#define MAX_INCLUDE_DEPTH   16

static void readlist( const char *listfile, struct params *params )
{
    static int include_depth = 0;
    static ADDR cur_segment_base = 0;   /* set by the 'g' command, file order */
    FILE *f;
    char buf[LINE_BUF_LEN + 1], *pbuf, *q;
    ADDR addr;
    int cmd;
    int n;
    char *notebuf = NULL;
    int notelength;
    int notebufsize = 0;
    unsigned int lineno = 0;
   
    enum {
        LINE_CMD,
        LINE_NOTE
    } linemode = LINE_CMD;
    
    if ( !listfile )
        error( "No listfile specifed" );

    if ( ++include_depth > MAX_INCLUDE_DEPTH )
        error( "Include nesting too deep (limit is %d)", MAX_INCLUDE_DEPTH );

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
            if ( *pbuf == '#' || *pbuf == '\n' || *pbuf == '\r' )
                continue;

            /* Remove trailing newline/carriage-return chars (handle CRLF) */
            if ( (q = strchr( pbuf, '\n' )) )
                *q = '\0';
            if ( (q = strchr( pbuf, '\r' )) )
                *q = '\0';

            /* Peel off command code, then do something about it */
            cmd = *pbuf++;
            cmd = tolower( (unsigned char)cmd ); /* Be case-agnostic */
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
            case 'u': /* widechar string dump        */
            case 'z': /* skip empty areas            */
                {
                    unsigned int cmd_idx = strchr( datchars, cmd ) - datchars;
                    unsigned bytes_per_line = BYTES_PER_LINE;
                    sscanf( pbuf, "%x%n", &addr, &n );
                    addr *= dasm_word_width_bytes;
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
                            { "CL",      1 },
                            { "BDATA",   1 }, 
                            { "STRING",  1 },
                            { NULL,      0 }, /* END */
                            { "WDATA",   1 },
                            { "CDATA",   1 },
                            { "PROC",    1 },
                            { "VCTR",    1 },
                            { "BMAP",    1 },
                            { "WSTRING", 1 },
                            { "SKIP",    1 }
                        };

                        if ( tbl[cmd_idx].pfx )
                            snprintf( pbuf, buf + sizeof(buf) - pbuf, GEN_LABEL_PREFIX "%s_%04d", tbl[cmd_idx].pfx, tbl[cmd_idx].num++ );
                    }
                    
                    /* Add a cross-ref entry for everything except an end entry */
                    if ( cmd != 'e' )
                        xref_addxreflabel( addr, pbuf );

                    addlist( &(params->cmdlist), 
                                addr, 
                                cmd_idx, 
                                bytes_per_line,
                                cur_segment_base,
                                pbuf );
                }
                break;

            case 'f':  /* inputfile */
                {
                    ADDR image_base = 0;
                    char *path = pbuf;
                    int has_base = 0;

                    SKIP_SPACE(path);
                    if ( *path == '@' )
                    {
                        path++;
                        if ( sscanf( path, "%x%n", &image_base, &n ) != 1 )
                            error( "%s(%u) :: Bad input image address", listfile, lineno );
                        path += n;
                        SKIP_SPACE(path);
                        if ( !*path )
                            error( "%s(%u) :: Missing input file name", listfile, lineno );
                        has_base = 1;
                    }
                    else
                    {
                        path = pbuf;
                    }

                    add_input_file( params, path, has_base, image_base );
                }
                break;

            case 'i':   /* include file */
                readlist( pbuf, params );
                break;

            case '>':   /* fast forward */
		sscanf( pbuf, "%x", &file_offset );
		break;

            case 'g':   /* segment base for following code entries */
                {
                    unsigned int seg;
                    sscanf( pbuf, "%x", &seg );
                    cur_segment_base = (ADDR)seg;
                }
                break;

            case 'o':   /* decoder option */
                {
                    SKIP_SPACE(pbuf);

                    if ( !strncmp( pbuf, "cpu=", 4 ) )
                    {
                        char *cpu = pbuf + 4;

                        if ( !strcmp( cpu, "default" ) || !strcmp( cpu, "max" ) )
                            dasm_cpu_level = 0;
                        else if ( !strcmp( cpu, "6502" ) )
                            dasm_cpu_level = CPU_6502;
                        else if ( !strcmp( cpu, "65c02" ) || !strcmp( cpu, "65C02" ) )
                            dasm_cpu_level = CPU_65C02;
                        else if ( !strcmp( cpu, "86" ) || !strcmp( cpu, "8086" ) || !strcmp( cpu, "8088" ) )
                            dasm_cpu_level = 8086;
                        else if ( !strcmp( cpu, "186" ) || !strcmp( cpu, "80186" ) || !strcmp( cpu, "80188" ) )
                            dasm_cpu_level = 80186;
                        else if ( !strcmp( cpu, "286" ) || !strcmp( cpu, "80286" ) )
                            dasm_cpu_level = 80286;
                        else if ( !strcmp( cpu, "386" ) || !strcmp( cpu, "80386" ) )
                            dasm_cpu_level = 80386;
                        else if ( !strcmp( cpu, "486" ) || !strcmp( cpu, "80486" ) )
                            dasm_cpu_level = 80486;
                        else if ( !strcmp( cpu, "68000" ) )
                            dasm_cpu_level = 68000;
                        else if ( !strcmp( cpu, "68010" ) )
                            dasm_cpu_level = 68010;
                        else if ( !strcmp( cpu, "68020" ) )
                            dasm_cpu_level = 68020;
                        else if ( !strcmp( cpu, "68030" ) )
                            dasm_cpu_level = 68030;
                        else if ( !strcmp( cpu, "68040" ) )
                            dasm_cpu_level = 68040;
                        else
                            error( "%s(%u) :: Unsupported CPU option '%s'", listfile, lineno, cpu );
                    }
                    else if ( !strncmp( pbuf, "fpu=", 4 ) )
                    {
                        char *fpu = pbuf + 4;

                        if ( !strcmp( fpu, "default" ) || !strcmp( fpu, "max" ) )
                            dasm_fpu_level = 0;
                        else if ( !strcmp( fpu, "none" ) )
                            dasm_fpu_level = 1;
                        else if ( !strcmp( fpu, "68881" ) )
                            dasm_fpu_level = 68881;
                        else if ( !strcmp( fpu, "68882" ) )
                            dasm_fpu_level = 68882;
                        else if ( !strcmp( fpu, "68040" ) )
                            dasm_fpu_level = 68040;
                        else
                            error( "%s(%u) :: Unsupported FPU option '%s'", listfile, lineno, fpu );
                    }
                    else
                    {
                        error( "%s(%u) :: Unsupported decoder option '%s'", listfile, lineno, pbuf );
                    }
                }
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
                    addr *= dasm_word_width_bytes;
                    pbuf += n;
                    
                    SKIP_SPACE(pbuf);
                    
                    if ( !*pbuf )
                    {
                        static unsigned int auto_label = 1;
                   
                        snprintf( pbuf, buf + sizeof(buf) - pbuf, "AL_%04d", auto_label++ );
                    }
                    
                    xref_addxreflabel( addr, pbuf );
                }
                break;

            case 'k':   /* Single-line (k)comment */
                {
                    sscanf( pbuf, "%x%n", &addr, &n );
                    addr *= dasm_word_width_bytes;
                    pbuf += n;
                    
                    SKIP_SPACE(pbuf);
                    if ( *pbuf )
                        addcomment( &linecmt, addr, pbuf );
                }
                break;

            case 'n':   /* Multiple-line note */
                {
                    sscanf( pbuf, "%x%n", &addr, &n );
                    addr *= dasm_word_width_bytes;
                    pbuf += n;

                    /* Initialise the note buffer and switch to LINE_NOTE mode. */
                    notelength = 0;
                    notebufsize = NOTE_BUF_INIT;
                    notebuf = realloc( notebuf, notebufsize );
                    if ( !notebuf )
                        error( "Out of memory for note buffer" );
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
                        strncpy( title, pbuf+1, sizeof(title) - 1 );
                        title[sizeof(title) - 1] = '\0';
                        if ( strlen(title) > 0 )
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
                    if ( isprint( (unsigned char)cmd ) )
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
                size_t linelen = strlen( pbuf );

                /* Grow buffer if needed */
                while ( notelength + linelen + 1 > notebufsize )
                {
                    notebufsize *= 2;
                    notebuf = realloc( notebuf, notebufsize );
                    if ( !notebuf )
                        error( "Out of memory for note buffer" );
                }
                memcpy( notebuf + notelength, pbuf, linelen + 1 );
                notelength += linelen;
            }
        }
    }

    free( notebuf );
    fclose( f );
    include_depth--;
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
    FILE *f = NULL;
    ADDR  addr;
    int   mode;
    unsigned int bpl;
    char *name;
    
    if ( !clist )
        error( "No disassembly commands specified" );

    load_input_images( params.inputfiles, clist->addr );
    
    addr  = clist->addr;
    mode  = clist->mode;
    name  = clist->name;
    bpl   = clist->bpl;
    dasm_segment_base = clist->seg;
    clist = clist->n;
    
    if ( input_image.file_count > 1 )
        printf( "%s   Processing %u input files (%lu bytes, %s)", COMMENT_DELIM, input_image.file_count, input_image.source_size, input_image.format );
    else if ( strcmp( input_image.format, "binary" ) == 0 )
        printf( "%s   Processing \"%s\" (%lu bytes)", COMMENT_DELIM, inputfile, input_image.source_size );
    else
        printf( "%s   Processing \"%s\" (%lu bytes, %s)", COMMENT_DELIM, inputfile, input_image.source_size, input_image.format );
    newline();
    if ( file_offset )
    {
         printf( "%s   File offset: 0x%04X", COMMENT_DELIM, file_offset ); newline();
    }
    printf( "%s   Disassembly start address: 0x%04X", COMMENT_DELIM, addr );              newline();
    printf( "%s   String terminator: 0x%02x", COMMENT_DELIM, string_terminator );         newline();
    newline();

    while ( clist )
    {
        if ( addr >= clist->addr )
        {
            if ( mode != clist->mode )
                newline();
            mode  = clist->mode;
            name  = clist->name;
            bpl   = clist->bpl;
            dasm_segment_base = clist->seg;
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
                        if ( isprint( (unsigned char)buf[p] ) )
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
                    if ( isprint( (unsigned char)buf[p] ) )
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
            dasm_segment_base = clist->seg;
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

                while ( addr < clist->addr && ( c = next( f, &addr ) ) )
                {
                    if ( c == string_terminator )
                        break;

                    if ( isprint( (unsigned char)c ) )
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
            dasm_segment_base = clist->seg;
            clist = clist->n;
        }
        else if ( mode == WSTRING )
        {
            /*****************************************************************
            *            u - WIDECHAR STRING DATA
            *****************************************************************/

            int c;

            newline();
            printcomment( blockcmt, addr, 0 );

            while ( addr < clist->addr )
            {
                emitaddr( addr, &params );
                if ( params.want_asm_out )
                    printf( params.want_stripped ? "   " : "\n   " );

                int in_quote = 0;
                printf( "DW      " );

                while ( addr < clist->addr && ( c = nextw( f, &addr ) ) )
                {
                    if ( c == string_terminator )
                        break;

                    if ( isprint( (unsigned char)c ) )
                    {
                        if ( !in_quote )
                        {
                            putchar( '\'' );
                            in_quote = 1;
                        }
                        putchar( c );
                    }
                    else
                    {
                        if ( in_quote )
                        {
                            printf( "', " );
                            in_quote = 0;
                        }
                        else
                        {
                            printf( ", " );
                        }
                        printf ("%#04X", (uint16_t) c );
                    }
                }
                if ( in_quote )
                    printf( "'" );
                newline();
            }

            mode = clist->mode;
            if ( mode == CODE || mode == PROCS )
                newline();
            name  = clist->name;
            bpl   = clist->bpl;
            dasm_segment_base = clist->seg;
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
            dasm_segment_base = clist->seg;
            clist = clist->n;
        }
        else if ( mode == SKIP )
        {
            /*****************************************************************
            *            z - SKIP
            *****************************************************************/

            int b, i = 0;

            newline();
            printcomment( blockcmt, addr, 0 );

            {
                emitaddr( addr, &params );
                if ( params.want_asm_out )
                    printf( params.want_stripped ? "   " : "\n   " );
            }

            while ( addr < clist->addr )
            {
                b = (unsigned char)next( f, &addr );
                if (b != 0)
                    error("Non-zero byte in skipped section %04x at %04x", addr, clist->addr);
                i++;
            }

            printf( "SKIP    %04x", i );
            newline();

            mode = clist->mode;
            if ( mode == CODE || mode == PROCS )
                newline();
            name  = clist->name;
            bpl   = clist->bpl;
            dasm_segment_base = clist->seg;
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
            dasm_segment_base = clist->seg;
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

                if ( isprint( (unsigned char)c ) )
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
            dasm_segment_base = clist->seg;
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
            dasm_segment_base = clist->seg;
            clist = clist->n;
        }
    } /* while() */
     
    free_input_image();
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

static void free_input_image( void )
{
    unsigned int i;

    for ( i = 0; i < input_image.count; i++ )
        free( input_image.segments[i].data );
    free( input_image.segments );
    memset( &input_image, 0, sizeof(input_image) );
}

static UBYTE *read_input_file( const char *path, unsigned long *size )
{
    FILE *f = fopen( path, "rb" );
    UBYTE *data;
    long len;

    if ( !f )
        error( "Failed to open input file \"%s\"", path );

    if ( fseek( f, 0, SEEK_END ) != 0 )
        error( "Failed to seek input file \"%s\"", path );
    len = ftell( f );
    if ( len < 0 )
        error( "Failed to measure input file \"%s\"", path );
    if ( fseek( f, 0, SEEK_SET ) != 0 )
        error( "Failed to rewind input file \"%s\"", path );

    data = zalloc( (size_t)len + 1 );
    if ( len && fread( data, 1, (size_t)len, f ) != (size_t)len )
        error( "Failed to read input file \"%s\"", path );
    fclose( f );

    *size = (unsigned long)len;
    return data;
}

static int ranges_overlap( ADDR a_start, unsigned int a_len, ADDR b_start, unsigned int b_len )
{
    ADDR a_end = a_start + a_len - 1;
    ADDR b_end = b_start + b_len - 1;

    return a_start <= b_end && b_start <= a_end;
}

static void input_add_segment( ADDR addr, const UBYTE *data, unsigned int len, const char *path, unsigned int line )
{
    struct input_segment *seg;
    unsigned int i;

    if ( len == 0 )
        return;
    if ( addr + len - 1 < addr )
        error( "%s(%u) :: Input record address range wraps", path, line );

    for ( i = 0; i < input_image.count; i++ )
        if ( ranges_overlap( addr, len, input_image.segments[i].addr, input_image.segments[i].len ) )
            error( "%s(%u) :: Input record overlaps existing data at 0x%04X", path, line, addr );

    if ( input_image.count == input_image.cap )
    {
        input_image.cap = input_image.cap ? input_image.cap * 2 : 32;
        input_image.segments = realloc( input_image.segments, input_image.cap * sizeof(*input_image.segments) );
        if ( !input_image.segments )
            error( "Out of memory" );
    }

    seg = &input_image.segments[input_image.count++];
    seg->addr = addr;
    seg->len = len;
    seg->data = zalloc( len );
    memcpy( seg->data, data, len );
}

static int input_try_read_at( ADDR addr, UBYTE *out )
{
    unsigned int i;

    for ( i = 0; i < input_image.count; i++ )
    {
        struct input_segment *seg = &input_image.segments[i];
        if ( addr >= seg->addr && addr - seg->addr < seg->len )
        {
            *out = seg->data[addr - seg->addr];
            return 1;
        }
    }
    return 0;
}

static UBYTE input_read_at( ADDR addr )
{
    UBYTE byte;

    if ( input_try_read_at( addr, &byte ) )
        return byte;
    error( "Input has no byte at address 0x%04X", addr );
    return 0;
}

static int hex_value( int c )
{
    if ( c >= '0' && c <= '9' )
        return c - '0';
    if ( c >= 'a' && c <= 'f' )
        return c - 'a' + 10;
    if ( c >= 'A' && c <= 'F' )
        return c - 'A' + 10;
    return -1;
}

static UBYTE parse_hex_byte( const char *path, unsigned int line, const char *p )
{
    int hi = hex_value( (unsigned char)p[0] );
    int lo = hex_value( (unsigned char)p[1] );

    if ( hi < 0 || lo < 0 )
        error( "%s(%u) :: Invalid hexadecimal digit", path, line );
    return (UBYTE)((hi << 4) | lo);
}

static void input_note_file( const char *format, unsigned long size )
{
    if ( input_image.file_count == 0 )
        input_image.format = format;
    else if ( strcmp( input_image.format, format ) != 0 )
        input_image.format = "mixed";

    input_image.file_count++;
    input_image.source_size += size;
}

static ADDR relocate_record_addr( const char *path, unsigned int line, ADDR base_addr, ADDR record_addr )
{
    ADDR addr = base_addr + record_addr;

    if ( addr < base_addr )
        error( "%s(%u) :: Input record address range wraps", path, line );

    return addr;
}

static void load_binary_image( const char *path, const UBYTE *data, unsigned long size, ADDR base_addr )
{
    if ( file_offset > size )
        error( "File offset 0x%04X is beyond end of input file", file_offset );
    if ( size - file_offset > UINT_MAX )
        error( "Input file \"%s\" is too large", path );

    input_add_segment( base_addr, data + file_offset, (unsigned int)(size - file_offset), path, 0 );
}

static void load_ihex_image( const char *path, UBYTE *data, ADDR base_addr )
{
    char *saveptr = NULL;
    char *line = strtok_r( (char *)data, "\n", &saveptr );
    unsigned int lineno = 0;
    ULWORD base = 0;

    if ( file_offset )
        error( "%s :: File offset command is only supported for binary input", path );

    while ( line )
    {
        char *p = line;
        UBYTE bytes[256];
        UBYTE count, type, checksum;
        unsigned int addr16, i, sum;

        lineno++;
        while ( isspace( (unsigned char)*p ) )
            p++;
        if ( *p == '\0' )
        {
            line = strtok_r( NULL, "\n", &saveptr );
            continue;
        }
        if ( *p != ':' )
            error( "%s(%u) :: Expected Intel HEX record", path, lineno );
        p++;

        count = parse_hex_byte( path, lineno, p ); p += 2;
        addr16 = parse_hex_byte( path, lineno, p ) << 8; p += 2;
        addr16 |= parse_hex_byte( path, lineno, p ); p += 2;
        type = parse_hex_byte( path, lineno, p ); p += 2;
        sum = count + ((addr16 >> 8) & 0xFF) + (addr16 & 0xFF) + type;

        for ( i = 0; i < count; i++, p += 2 )
        {
            bytes[i] = parse_hex_byte( path, lineno, p );
            sum += bytes[i];
        }
        checksum = parse_hex_byte( path, lineno, p );
        sum += checksum;
        if ( (sum & 0xFF) != 0 )
            error( "%s(%u) :: Bad Intel HEX checksum", path, lineno );

        switch ( type )
        {
        case 0x00:
            input_add_segment( relocate_record_addr( path, lineno, base_addr, (ADDR)(base + addr16) ), bytes, count, path, lineno );
            break;
        case 0x01:
            return;
        case 0x02:
            if ( count != 2 )
                error( "%s(%u) :: Bad Intel HEX extended segment record length", path, lineno );
            base = (((ULWORD)bytes[0] << 8) | bytes[1]) << 4;
            break;
        case 0x04:
            if ( count != 2 )
                error( "%s(%u) :: Bad Intel HEX extended linear record length", path, lineno );
            base = (((ULWORD)bytes[0] << 8) | bytes[1]) << 16;
            break;
        case 0x03:
        case 0x05:
            break;
        default:
            error( "%s(%u) :: Unsupported Intel HEX record type %02X", path, lineno, type );
        }

        line = strtok_r( NULL, "\n", &saveptr );
    }
}

static void load_srec_image( const char *path, UBYTE *data, ADDR base_addr )
{
    char *saveptr = NULL;
    char *line = strtok_r( (char *)data, "\n", &saveptr );
    unsigned int lineno = 0;

    if ( file_offset )
        error( "%s :: File offset command is only supported for binary input", path );

    while ( line )
    {
        char *p = line;
        int type;
        int addr_len = 0;
        UBYTE bytes[256];
        UBYTE count;
        ULWORD addr = 0;
        unsigned int i, data_len, sum;

        lineno++;
        while ( isspace( (unsigned char)*p ) )
            p++;
        if ( *p == '\0' )
        {
            line = strtok_r( NULL, "\n", &saveptr );
            continue;
        }
        if ( p[0] != 'S' || !isdigit( (unsigned char)p[1] ) )
            error( "%s(%u) :: Expected Motorola S-record", path, lineno );
        type = p[1] - '0';
        p += 2;

        count = parse_hex_byte( path, lineno, p ); p += 2;
        sum = count;
        for ( i = 0; i < count; i++, p += 2 )
        {
            bytes[i] = parse_hex_byte( path, lineno, p );
            sum += bytes[i];
        }
        if ( (sum & 0xFF) != 0xFF )
            error( "%s(%u) :: Bad Motorola S-record checksum", path, lineno );

        if ( type == 1 || type == 9 )
            addr_len = 2;
        else if ( type == 2 || type == 8 )
            addr_len = 3;
        else if ( type == 3 || type == 7 )
            addr_len = 4;
        else if ( type == 0 || type == 5 || type == 6 )
        {
            line = strtok_r( NULL, "\n", &saveptr );
            continue;
        }
        else
            error( "%s(%u) :: Unsupported Motorola S-record type S%d", path, lineno, type );

        if ( count < (unsigned int)addr_len + 1 )
            error( "%s(%u) :: Bad Motorola S-record byte count", path, lineno );
        for ( i = 0; i < (unsigned int)addr_len; i++ )
            addr = (addr << 8) | bytes[i];

        data_len = count - addr_len - 1;
        if ( type == 1 || type == 2 || type == 3 )
            input_add_segment( relocate_record_addr( path, lineno, base_addr, (ADDR)addr ), bytes + addr_len, data_len, path, lineno );

        line = strtok_r( NULL, "\n", &saveptr );
    }
}

static void load_one_input_image( struct input_file *file, ADDR default_binary_base_addr )
{
    const char *path = file->path;
    UBYTE *data;
    unsigned long size;
    unsigned long i = 0;
    unsigned int segment_count;
    ADDR base_addr = file->has_base ? file->base : 0;

    data = read_input_file( path, &size );
    segment_count = input_image.count;

    while ( i < size && isspace( (unsigned char)data[i] ) )
        i++;

    if ( i < size && data[i] == ':' )
    {
        input_note_file( "Intel HEX", size );
        load_ihex_image( path, data, base_addr );
    }
    else if ( i + 1 < size && data[i] == 'S' && isdigit( (unsigned char)data[i + 1] ) )
    {
        input_note_file( "Motorola S-record", size );
        load_srec_image( path, data, base_addr );
    }
    else
    {
        input_note_file( "binary", size );
        load_binary_image( path, data, size, file->has_base ? file->base : default_binary_base_addr );
    }

    free( data );
    if ( input_image.count == segment_count )
        error( "Input file \"%s\" contains no data records", path );
}

static void load_input_images( struct input_file *files, ADDR default_binary_base_addr )
{
    struct input_file *file;

    free_input_image();
    for ( file = files; file; file = file->next )
        load_one_input_image( file, default_binary_base_addr );

    if ( input_image.count == 0 )
        error( "No input data loaded" );
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
    UBYTE c;

    (void)fp;
    c = input_read_at( *addr );
        
    if ( insn_byte_idx < dasm_max_insn_length )
        insn_byte_buffer[insn_byte_idx++] = c;
    
    (*addr)++;
    input_peek_addr = *addr;
    return c;
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

    (void)fp;
    lo = input_read_at( *addr );
    hi = input_read_at( *addr + 1 );
        
    if ( insn_byte_idx < dasm_max_insn_length )
        insn_byte_buffer[insn_byte_idx++] = (UBYTE)hi;
        
    if ( insn_byte_idx < dasm_max_insn_length )
        insn_byte_buffer[insn_byte_idx++] = (UBYTE)lo;
    
    (*addr)++;
    (*addr)++;
    input_peek_addr = *addr;
    
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
    UBYTE c;

    (void)fp;
    c = input_read_at( input_peek_addr );
    
    return c;
}

UWORD peekw( FILE *fp )
{
    UBYTE lo, hi;
    UWORD w;

    (void)fp;
    if ( !input_try_read_at( input_peek_addr, &lo ) || !input_try_read_at( input_peek_addr + 1, &hi ) )
        return 0;

    if ( dasm_word_msb_first )
        SWAP( lo, hi );

    w = ( ( hi & 0xFF ) << 8 ) | ( lo & 0xFF );
    return w;
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

    if ( !params.inputfiles )
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
