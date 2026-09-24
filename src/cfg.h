#ifndef _CFG_H_
#define _CFG_H_

#include <stdio.h>

#include "dasmxx.h"

typedef enum {
    CFG_ROOT_CODE,
    CFG_ROOT_PROC,
    CFG_ROOT_VECTOR
} CFG_ROOT_KIND;

typedef enum {
    CFG_EDGE_FALLTHROUGH,
    CFG_EDGE_BRANCH,
    CFG_EDGE_CALL,
    CFG_EDGE_RETURN,
    CFG_EDGE_STOP,
    CFG_EDGE_INDIRECT
} CFG_EDGE_KIND;

struct cfg;

struct cfg *cfg_create( void );
void cfg_free( struct cfg *cfg );
void cfg_add_root( struct cfg *cfg, ADDR addr, CFG_ROOT_KIND kind );
void cfg_trace( struct cfg *cfg );
void cfg_emit_debug( const struct cfg *cfg, FILE *out );
void cfg_emit_cmd( const struct cfg *cfg, FILE *out );
void cfg_emit_dot( const struct cfg *cfg, FILE *out );
void cfg_emit_json( const struct cfg *cfg, FILE *out );

const char *cfg_default_extension( const char *format );
char *cfg_default_output_path( const char *listfile, const char *format );

#endif
