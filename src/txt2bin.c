/*****************************************************************************
 *
 * Copyright (C) 2016, Neil Johnson
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
#include <stdlib.h>
#include <ctype.h>

/**
 Return the hex value of a hex char.
 **/
int hexval( int c )
{
    if ( isdigit( c ) )
        c -= '0';
    else 
        c = toupper( c ) - 'A' + 10;
    
    return c;
}

/**
 Print "syntx error" and exit.
 **/
void synerr( void )
{
    fprintf( stderr, "syntax error\n" );
    exit( EXIT_FAILURE );
}

/**
 Print "file error" and exit.
 **/
void filerr( void )
{
    fprintf( stderr, "file error\n" );
    exit( EXIT_FAILURE );
}

/**
 txt2bin - convert textual hex values into raw binary
 
 Usage:
    txt2bin infile.txt outfile.bin
    
 **/
int main( int argc, char *argv[] )
{
    FILE *fin;
    FILE *fout;
    int c;
    int byte;
    enum {
        S_SOL,  /* Start Of Line */
        S_1ST,  /* Seen first byte */
        S_SKIP  /* Skip to EOL */
    } state;
    
    /*
     * Check args and open files
     */
    if ( argc != 3 )
    {
        fprintf( stderr, "txt2bin <input.txt> <output.bin>\n" );
        EXIT_FAILURE;
    }
    
    fin = fopen( argv[1], "r" );
    if ( !fin )
    {
        fprintf( stderr, "txt2bin: failed to open \"%s\"\n", argv[1] );
        return EXIT_FAILURE;
    }
    
    fout = fopen( argv[2], "wb" );
    if ( !fout )
    {
        fprintf( stderr, "txt2bin: failed to open \"%s\"\n", argv[2] );
        return EXIT_FAILURE;
    }
    
    /*
     * Run state machine on input stream
     */
    byte = 0;
    state = S_SOL;
    while ( ( c = fgetc( fin ) ) != EOF )
    {
        switch ( state )
        {
case S_SOL:
            if ( isspace( c ) )
                continue;
            else if ( c == '#' )
                state = S_SKIP;
            else if ( isxdigit( c ) )
            {
                byte = hexval( c );
                state = S_1ST;
            }
            else
                synerr();
            break;
            
case S_1ST:
            if ( isxdigit( c ) )
            {
                byte = ( byte << 4 ) + hexval( c );
                if ( 1 != fwrite( &byte, 1, 1, fout ) )
                    filerr();
                byte = 0;
                state = S_SOL;
            }
            else
                synerr();
            break;

case S_SKIP:
            if ( c == '\n' )
                state = S_SOL;
            break;
            
default:
            synerr();
            break;
        }
    }
    
    /*
     * All done, close files, and return successfully.
     */
    fclose( fin );
    fclose( fout );
    
    return EXIT_SUCCESS;
}
