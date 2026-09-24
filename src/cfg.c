#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfg.h"

struct cfg_root {
    ADDR addr;
    CFG_ROOT_KIND kind;
};

struct cfg_insn {
    struct dasm_insn_info info;
};

struct cfg_edge {
    ADDR from;
    ADDR to;
    int has_to;
    CFG_EDGE_KIND kind;
};

struct cfg_block {
    ADDR start;
    ADDR end;
};

struct cfg {
    struct cfg_root *roots;
    unsigned int root_count;
    unsigned int root_cap;

    struct cfg_insn *insns;
    unsigned int insn_count;
    unsigned int insn_cap;

    struct cfg_edge *edges;
    unsigned int edge_count;
    unsigned int edge_cap;

    struct cfg_block *blocks;
    unsigned int block_count;
    unsigned int block_cap;
};

static const char *root_kind_name( CFG_ROOT_KIND kind )
{
    switch ( kind )
    {
    case CFG_ROOT_CODE:   return "code";
    case CFG_ROOT_PROC:   return "proc";
    case CFG_ROOT_VECTOR: return "vector";
    }
    return "unknown";
}

static const char *edge_kind_name( CFG_EDGE_KIND kind )
{
    switch ( kind )
    {
    case CFG_EDGE_FALLTHROUGH: return "fallthrough";
    case CFG_EDGE_BRANCH:      return "branch";
    case CFG_EDGE_CALL:        return "call";
    case CFG_EDGE_RETURN:      return "return";
    case CFG_EDGE_RESUME:      return "resume";
    case CFG_EDGE_STOP:        return "stop";
    case CFG_EDGE_INDIRECT:    return "indirect";
    }
    return "unknown";
}

static void *grow_array( void *ptr, unsigned int *cap, unsigned int item_size )
{
    if ( *cap == 0 )
        *cap = 32;
    else
        *cap *= 2;

    ptr = realloc( ptr, (*cap) * item_size );
    if ( !ptr )
        error( "Out of memory" );
    return ptr;
}

static int root_exists( const struct cfg *cfg, ADDR addr, CFG_ROOT_KIND kind )
{
    unsigned int i;

    for ( i = 0; i < cfg->root_count; i++ )
        if ( cfg->roots[i].addr == addr && cfg->roots[i].kind == kind )
            return 1;
    return 0;
}

void cfg_add_root( struct cfg *cfg, ADDR addr, CFG_ROOT_KIND kind )
{
    if ( root_exists( cfg, addr, kind ) )
        return;

    if ( cfg->root_count == cfg->root_cap )
        cfg->roots = grow_array( cfg->roots, &cfg->root_cap, sizeof(*cfg->roots) );

    cfg->roots[cfg->root_count].addr = addr;
    cfg->roots[cfg->root_count].kind = kind;
    cfg->root_count++;
}

struct cfg *cfg_create( void )
{
    return zalloc( sizeof(struct cfg) );
}

void cfg_free( struct cfg *cfg )
{
    if ( !cfg )
        return;
    free( cfg->roots );
    free( cfg->insns );
    free( cfg->edges );
    free( cfg->blocks );
    free( cfg );
}

static int insn_cmp( const void *a, const void *b )
{
    const struct cfg_insn *ia = a;
    const struct cfg_insn *ib = b;

    if ( ia->info.addr < ib->info.addr )
        return -1;
    if ( ia->info.addr > ib->info.addr )
        return 1;
    return 0;
}

static int edge_cmp( const void *a, const void *b )
{
    const struct cfg_edge *ea = a;
    const struct cfg_edge *eb = b;

    if ( ea->from < eb->from )
        return -1;
    if ( ea->from > eb->from )
        return 1;
    if ( ea->has_to != eb->has_to )
        return ea->has_to - eb->has_to;
    if ( ea->has_to && ea->to != eb->to )
        return ea->to < eb->to ? -1 : 1;
    return (int)ea->kind - (int)eb->kind;
}

static struct cfg_insn *find_insn( const struct cfg *cfg, ADDR addr )
{
    unsigned int i;

    for ( i = 0; i < cfg->insn_count; i++ )
        if ( cfg->insns[i].info.addr == addr )
            return &cfg->insns[i];
    return NULL;
}

static int edge_exists( const struct cfg *cfg, ADDR from, int has_to, ADDR to, CFG_EDGE_KIND kind )
{
    unsigned int i;

    for ( i = 0; i < cfg->edge_count; i++ )
        if ( cfg->edges[i].from == from
             && cfg->edges[i].has_to == has_to
             && (!has_to || cfg->edges[i].to == to)
             && cfg->edges[i].kind == kind )
            return 1;
    return 0;
}

static void add_edge( struct cfg *cfg, ADDR from, int has_to, ADDR to, CFG_EDGE_KIND kind )
{
    if ( edge_exists( cfg, from, has_to, to, kind ) )
        return;

    if ( cfg->edge_count == cfg->edge_cap )
        cfg->edges = grow_array( cfg->edges, &cfg->edge_cap, sizeof(*cfg->edges) );

    cfg->edges[cfg->edge_count].from = from;
    cfg->edges[cfg->edge_count].has_to = has_to;
    cfg->edges[cfg->edge_count].to = to;
    cfg->edges[cfg->edge_count].kind = kind;
    cfg->edge_count++;
}

static void add_insn( struct cfg *cfg, const struct dasm_insn_info *info )
{
    if ( find_insn( cfg, info->addr ) )
        return;

    if ( cfg->insn_count == cfg->insn_cap )
        cfg->insns = grow_array( cfg->insns, &cfg->insn_cap, sizeof(*cfg->insns) );

    cfg->insns[cfg->insn_count].info = *info;
    cfg->insn_count++;
}

static int boundary_exists( const ADDR *boundaries, unsigned int count, ADDR addr )
{
    unsigned int i;

    for ( i = 0; i < count; i++ )
        if ( boundaries[i] == addr )
            return 1;
    return 0;
}

static void add_boundary( ADDR **boundaries, unsigned int *count, unsigned int *cap, ADDR addr )
{
    if ( boundary_exists( *boundaries, *count, addr ) )
        return;

    if ( *count == *cap )
        *boundaries = grow_array( *boundaries, cap, sizeof(**boundaries) );

    (*boundaries)[(*count)++] = addr;
}

static int addr_cmp( const void *a, const void *b )
{
    ADDR aa = *(const ADDR *)a;
    ADDR bb = *(const ADDR *)b;

    if ( aa < bb )
        return -1;
    if ( aa > bb )
        return 1;
    return 0;
}

static void enqueue( ADDR **work, unsigned int *count, unsigned int *cap, ADDR addr )
{
    if ( !dasm_input_mapped( addr ) )
        return;

    if ( *count == *cap )
        *work = grow_array( *work, cap, sizeof(**work) );
    (*work)[(*count)++] = addr;
}

static void add_info_edges_and_work( struct cfg *cfg, const struct dasm_insn_info *info,
                                     ADDR **work, unsigned int *work_count, unsigned int *work_cap )
{
    unsigned int i;

    switch ( info->flow )
    {
    case CFG_FLOW_NORMAL:
        add_edge( cfg, info->addr, 1, info->next, CFG_EDGE_FALLTHROUGH );
        enqueue( work, work_count, work_cap, info->next );
        break;

    case CFG_FLOW_JUMP:
        for ( i = 0; i < info->target_count; i++ )
        {
            add_edge( cfg, info->addr, 1, info->targets[i], CFG_EDGE_BRANCH );
            enqueue( work, work_count, work_cap, info->targets[i] );
        }
        if ( info->target_count == 0 )
            add_edge( cfg, info->addr, 0, 0, CFG_EDGE_INDIRECT );
        break;

    case CFG_FLOW_COND_JUMP:
        for ( i = 0; i < info->target_count; i++ )
        {
            add_edge( cfg, info->addr, 1, info->targets[i], CFG_EDGE_BRANCH );
            enqueue( work, work_count, work_cap, info->targets[i] );
        }
        add_edge( cfg, info->addr, 1, info->next, CFG_EDGE_FALLTHROUGH );
        enqueue( work, work_count, work_cap, info->next );
        break;

    case CFG_FLOW_CALL:
    case CFG_FLOW_COND_CALL:
        for ( i = 0; i < info->target_count; i++ )
        {
            add_edge( cfg, info->addr, 1, info->targets[i], CFG_EDGE_CALL );
            enqueue( work, work_count, work_cap, info->targets[i] );
        }
        add_edge( cfg, info->addr, 1, info->next, CFG_EDGE_FALLTHROUGH );
        enqueue( work, work_count, work_cap, info->next );
        break;

    case CFG_FLOW_RETURN:
        add_edge( cfg, info->addr, 0, 0, CFG_EDGE_RETURN );
        break;

    case CFG_FLOW_COND_RETURN:
        add_edge( cfg, info->addr, 0, 0, CFG_EDGE_RETURN );
        add_edge( cfg, info->addr, 1, info->next, CFG_EDGE_FALLTHROUGH );
        enqueue( work, work_count, work_cap, info->next );
        break;

    case CFG_FLOW_HALT:
        add_edge( cfg, info->addr, 1, info->next, CFG_EDGE_RESUME );
        enqueue( work, work_count, work_cap, info->next );
        break;

    case CFG_FLOW_STOP:
        add_edge( cfg, info->addr, 0, 0, CFG_EDGE_STOP );
        break;

    case CFG_FLOW_INDIRECT_JUMP:
    case CFG_FLOW_INDIRECT_CALL:
        add_edge( cfg, info->addr, 0, 0, CFG_EDGE_INDIRECT );
        break;
    }
}

static int flow_ends_block( CFG_FLOW flow )
{
    return flow != CFG_FLOW_NORMAL;
}

static int block_boundary_at( const ADDR *boundaries, unsigned int count, ADDR addr )
{
    return boundary_exists( boundaries, count, addr );
}

static void add_block( struct cfg *cfg, ADDR start, ADDR end )
{
    if ( cfg->block_count == cfg->block_cap )
        cfg->blocks = grow_array( cfg->blocks, &cfg->block_cap, sizeof(*cfg->blocks) );

    cfg->blocks[cfg->block_count].start = start;
    cfg->blocks[cfg->block_count].end = end;
    cfg->block_count++;
}

static void build_blocks( struct cfg *cfg )
{
    ADDR *boundaries = NULL;
    unsigned int boundary_count = 0;
    unsigned int boundary_cap = 0;
    unsigned int i;
    ADDR block_start = 0;
    ADDR block_end = 0;
    int in_block = 0;

    free( cfg->blocks );
    cfg->blocks = NULL;
    cfg->block_count = 0;
    cfg->block_cap = 0;

    for ( i = 0; i < cfg->root_count; i++ )
        add_boundary( &boundaries, &boundary_count, &boundary_cap, cfg->roots[i].addr );

    for ( i = 0; i < cfg->edge_count; i++ )
        if ( cfg->edges[i].has_to )
        {
            struct cfg_insn *source = find_insn( cfg, cfg->edges[i].from );

            if ( cfg->edges[i].kind != CFG_EDGE_FALLTHROUGH
                 || (source && source->info.flow != CFG_FLOW_NORMAL) )
                add_boundary( &boundaries, &boundary_count, &boundary_cap, cfg->edges[i].to );
        }

    qsort( boundaries, boundary_count, sizeof(*boundaries), addr_cmp );
    qsort( cfg->insns, cfg->insn_count, sizeof(*cfg->insns), insn_cmp );

    for ( i = 0; i < cfg->insn_count; i++ )
    {
        struct dasm_insn_info *info = &cfg->insns[i].info;

        if ( !in_block )
        {
            block_start = info->addr;
            in_block = 1;
        }
        else if ( block_boundary_at( boundaries, boundary_count, info->addr )
                  || block_end != info->addr )
        {
            add_block( cfg, block_start, block_end );
            block_start = info->addr;
        }

        block_end = info->next;

        if ( flow_ends_block( info->flow ) )
        {
            add_block( cfg, block_start, block_end );
            in_block = 0;
        }
    }

    if ( in_block )
        add_block( cfg, block_start, block_end );

    free( boundaries );
}

void cfg_trace( struct cfg *cfg )
{
    ADDR *work = NULL;
    unsigned int work_count = 0;
    unsigned int work_cap = 0;
    unsigned int i;

    if ( !dasm_cfg_supported() )
        error( "CFG tracing is not supported by this decoder yet" );

    for ( i = 0; i < cfg->root_count; i++ )
        enqueue( &work, &work_count, &work_cap, cfg->roots[i].addr );

    xref_set_suppressed( 1 );
    while ( work_count )
    {
        ADDR addr = work[--work_count];

        while ( dasm_input_mapped( addr ) && !find_insn( cfg, addr ) )
        {
            char outbuf[256];
            struct dasm_insn_info info;

            memset( &info, 0, sizeof(info) );
            info.addr = addr;
            info.flow = CFG_FLOW_NORMAL;
            dasm_set_insn_info( &info );
            info.next = dasm_insn( NULL, outbuf, addr );
            dasm_set_insn_info( NULL );
            strncpy( info.text, outbuf, sizeof(info.text) - 1 );
            info.text[sizeof(info.text) - 1] = '\0';

            add_insn( cfg, &info );
            add_info_edges_and_work( cfg, &info, &work, &work_count, &work_cap );

            if ( flow_ends_block( info.flow ) )
                break;
            addr = info.next;
        }
    }
    xref_set_suppressed( 0 );

    qsort( cfg->edges, cfg->edge_count, sizeof(*cfg->edges), edge_cmp );
    build_blocks( cfg );
    free( work );
}

static const struct cfg_block *find_block_for_addr( const struct cfg *cfg, ADDR addr )
{
    unsigned int i;

    for ( i = 0; i < cfg->block_count; i++ )
        if ( cfg->blocks[i].start == addr )
            return &cfg->blocks[i];
    return NULL;
}

static const struct cfg_block *find_block_containing_addr( const struct cfg *cfg, ADDR addr )
{
    unsigned int i;

    for ( i = 0; i < cfg->block_count; i++ )
        if ( addr >= cfg->blocks[i].start && addr < cfg->blocks[i].end )
            return &cfg->blocks[i];
    return NULL;
}

static void dot_escape( FILE *out, const char *s );
static void json_escape( FILE *out, const char *s );

static int insn_in_block( const struct cfg_insn *insn, const struct cfg_block *block )
{
    return insn->info.addr >= block->start && insn->info.addr < block->end;
}

static int flow_is_conditional( CFG_FLOW flow )
{
    return flow == CFG_FLOW_COND_JUMP
           || flow == CFG_FLOW_COND_CALL
           || flow == CFG_FLOW_COND_RETURN;
}

static const struct dasm_insn_info *block_last_insn( const struct cfg *cfg, const struct cfg_block *block )
{
    const struct dasm_insn_info *last = NULL;
    unsigned int i;

    for ( i = 0; i < cfg->insn_count; i++ )
        if ( insn_in_block( &cfg->insns[i], block ) )
            last = &cfg->insns[i].info;
    return last;
}

void cfg_emit_debug( const struct cfg *cfg, FILE *out )
{
    unsigned int i;

    fprintf( out, "cfg format=debug target=%s\n", dasm_name );
    for ( i = 0; i < cfg->root_count; i++ )
    {
        const char *label = xref_findaddrlabel( cfg->roots[i].addr );

        fprintf( out, "root addr=%04X kind=%s", cfg->roots[i].addr, root_kind_name( cfg->roots[i].kind ) );
        if ( label )
            fprintf( out, " label=\"%s\"", label );
        fprintf( out, "\n" );
    }

    for ( i = 0; i < cfg->block_count; i++ )
    {
        const char *label = xref_findaddrlabel( cfg->blocks[i].start );
        unsigned int j;

        fprintf( out, "block start=%04X end=%04X", cfg->blocks[i].start, cfg->blocks[i].end );
        if ( label )
            fprintf( out, " label=\"%s\"", label );
        fprintf( out, "\n" );
        for ( j = 0; j < cfg->insn_count; j++ )
            if ( insn_in_block( &cfg->insns[j], &cfg->blocks[i] ) )
                fprintf( out, "block_insn block=%04X addr=%04X text=\"%s\"\n",
                         cfg->blocks[i].start, cfg->insns[j].info.addr, cfg->insns[j].info.text );
    }

    for ( i = 0; i < cfg->insn_count; i++ )
    {
        const struct dasm_insn_info *info = &cfg->insns[i].info;
        unsigned int t;

        fprintf( out, "insn addr=%04X next=%04X flow=%s text=\"%s\"",
                 info->addr, info->next, dasm_cfg_flow_name( info->flow ), info->text );
        for ( t = 0; t < info->target_count; t++ )
            fprintf( out, " target%u=%04X", t, info->targets[t] );
        fprintf( out, "\n" );
    }

    for ( i = 0; i < cfg->edge_count; i++ )
    {
        const struct cfg_edge *edge = &cfg->edges[i];

        if ( edge->has_to )
            fprintf( out, "edge from=%04X to=%04X kind=%s\n",
                     edge->from, edge->to, edge_kind_name( edge->kind ) );
        else
            fprintf( out, "edge from=%04X kind=%s\n",
                     edge->from, edge_kind_name( edge->kind ) );
    }
}

void cfg_emit_cmd( const struct cfg *cfg, FILE *out )
{
    unsigned int i;

    fprintf( out, "# CFG discovered by %s\n", dasm_name );
    for ( i = 0; i < cfg->root_count; i++ )
        fprintf( out, "# root %04X %s\n", cfg->roots[i].addr, root_kind_name( cfg->roots[i].kind ) );

    for ( i = 0; i < cfg->block_count; i++ )
    {
        const char *label = xref_findaddrlabel( cfg->blocks[i].start );
        fprintf( out, "c%04X", cfg->blocks[i].start / dasm_word_width_bytes );
        if ( label )
            fprintf( out, "    %s", label );
        fprintf( out, "\n" );
    }
}

void cfg_emit_dot( const struct cfg *cfg, FILE *out )
{
    unsigned int i;

    fprintf( out, "digraph cfg {\n" );
    fprintf( out, "  graph [label=\"%s CFG\", labelloc=t];\n", dasm_name );
    fprintf( out, "  node [fontname=\"Courier\"];\n" );

    for ( i = 0; i < cfg->block_count; i++ )
    {
        const struct dasm_insn_info *last = block_last_insn( cfg, &cfg->blocks[i] );
        const char *label = xref_findaddrlabel( cfg->blocks[i].start );
        unsigned int j;

        fprintf( out, "  \"B_%04X\" [shape=box, label=\"",
                 cfg->blocks[i].start );
        if ( label )
        {
            dot_escape( out, label );
            fprintf( out, ":\\l" );
        }
        for ( j = 0; j < cfg->insn_count; j++ )
            if ( insn_in_block( &cfg->insns[j], &cfg->blocks[i] )
                 && !(last == &cfg->insns[j].info && flow_is_conditional( last->flow )) )
            {
                fprintf( out, "%04X: ", cfg->insns[j].info.addr );
                dot_escape( out, cfg->insns[j].info.text );
                fprintf( out, "\\l" );
            }
        fprintf( out, "\"];\n" );

        if ( last && flow_is_conditional( last->flow ) )
        {
            fprintf( out, "  \"D_%04X\" [shape=diamond, label=\"%04X: ",
                     last->addr, last->addr );
            dot_escape( out, last->text );
            fprintf( out, "\"];\n"
                          "  \"B_%04X\" -> \"D_%04X\" [label=\"condition\"];\n",
                     cfg->blocks[i].start, last->addr );
        }
    }

    for ( i = 0; i < cfg->root_count; i++ )
        if ( find_block_for_addr( cfg, cfg->roots[i].addr ) )
        {
            const char *label = xref_findaddrlabel( cfg->roots[i].addr );

            fprintf( out, "  \"R_%u\" [shape=ellipse, label=\"%s\\n",
                     i, root_kind_name( cfg->roots[i].kind ) );
            if ( label )
            {
                dot_escape( out, label );
                fprintf( out, "\\n" );
            }
            fprintf( out, "%04X\"];\n"
                          "  \"R_%u\" -> \"B_%04X\" [label=\"entry\"];\n",
                     cfg->roots[i].addr, i, cfg->roots[i].addr );
        }

    for ( i = 0; i < cfg->edge_count; i++ )
    {
        const struct cfg_edge *edge = &cfg->edges[i];
        const struct cfg_block *from_block = NULL;
        const struct cfg_block *to_block = NULL;
        const struct dasm_insn_info *last;
        char from_node[32];
        unsigned int b;

        for ( b = 0; b < cfg->block_count; b++ )
            if ( edge->from >= cfg->blocks[b].start && edge->from < cfg->blocks[b].end )
            {
                from_block = &cfg->blocks[b];
                break;
            }

        if ( !from_block )
            continue;

        last = block_last_insn( cfg, from_block );
        if ( last && flow_is_conditional( last->flow ) && edge->from == last->addr )
            sprintf( from_node, "D_%04X", last->addr );
        else
            sprintf( from_node, "B_%04X", from_block->start );

        if ( edge->has_to )
        {
            to_block = find_block_for_addr( cfg, edge->to );
            if ( to_block )
                fprintf( out, "  \"%s\" -> \"B_%04X\" [label=\"%s\"];\n",
                         from_node, to_block->start, edge_kind_name( edge->kind ) );
            else if ( find_block_containing_addr( cfg, edge->to ) == from_block )
            {
                continue;
            }
            else
                fprintf( out, "  \"%s\" -> \"U_%04X\" [label=\"%s\"];\n"
                              "  \"U_%04X\" [label=\"%04X\", shape=box, style=dashed];\n",
                         from_node, edge->to, edge_kind_name( edge->kind ),
                         edge->to, edge->to );
        }
        else
        {
            fprintf( out, "  \"%s\" -> \"X_%04X_%s\" [label=\"%s\"];\n"
                          "  \"X_%04X_%s\" [label=\"%s\", shape=box, style=dashed];\n",
                     from_node, edge->from, edge_kind_name( edge->kind ), edge_kind_name( edge->kind ),
                     edge->from, edge_kind_name( edge->kind ), edge_kind_name( edge->kind ) );
        }
    }

    fprintf( out, "}\n" );
}

static void json_escape( FILE *out, const char *s )
{
    for ( ; *s; s++ )
    {
        if ( *s == '"' || *s == '\\' )
            fputc( '\\', out );
        if ( *s == '\n' )
            fprintf( out, "\\n" );
        else
            fputc( *s, out );
    }
}

static void dot_escape( FILE *out, const char *s )
{
    for ( ; *s; s++ )
    {
        if ( *s == '"' || *s == '\\' || *s == '{' || *s == '}' || *s == '<' || *s == '>' )
            fputc( '\\', out );
        if ( *s == '\n' )
            fprintf( out, "\\n" );
        else
            fputc( *s, out );
    }
}

void cfg_emit_json( const struct cfg *cfg, FILE *out )
{
    unsigned int i;

    fprintf( out, "{\n  \"target\": \"%s\",\n  \"roots\": [\n", dasm_name );
    for ( i = 0; i < cfg->root_count; i++ )
    {
        const char *label = xref_findaddrlabel( cfg->roots[i].addr );

        fprintf( out, "    {\"addr\": %u, \"addr_hex\": \"%04X\", \"kind\": \"%s\"",
                 cfg->roots[i].addr, cfg->roots[i].addr, root_kind_name( cfg->roots[i].kind ) );
        if ( label )
        {
            fprintf( out, ", \"label\": \"" );
            json_escape( out, label );
            fprintf( out, "\"" );
        }
        fprintf( out, "}%s\n", i + 1 == cfg->root_count ? "" : "," );
    }

    fprintf( out, "  ],\n  \"blocks\": [\n" );
    for ( i = 0; i < cfg->block_count; i++ )
    {
        const char *label = xref_findaddrlabel( cfg->blocks[i].start );
        unsigned int j;
        int first = 1;

        fprintf( out, "    {\"start\": %u, \"start_hex\": \"%04X\", \"end\": %u, \"end_hex\": \"%04X\"",
                 cfg->blocks[i].start, cfg->blocks[i].start, cfg->blocks[i].end, cfg->blocks[i].end );
        if ( label )
        {
            fprintf( out, ", \"label\": \"" );
            json_escape( out, label );
            fprintf( out, "\"" );
        }
        fprintf( out, ", \"instructions\": [" );
        for ( j = 0; j < cfg->insn_count; j++ )
        {
            if ( !insn_in_block( &cfg->insns[j], &cfg->blocks[i] ) )
                continue;

            fprintf( out, "%s{\"addr\": %u, \"addr_hex\": \"%04X\", \"text\": \"",
                     first ? "" : ", ", cfg->insns[j].info.addr, cfg->insns[j].info.addr );
            json_escape( out, cfg->insns[j].info.text );
            fprintf( out, "\"}" );
            first = 0;
        }
        fprintf( out, "]}%s\n", i + 1 == cfg->block_count ? "" : "," );
    }

    fprintf( out, "  ],\n  \"instructions\": [\n" );
    for ( i = 0; i < cfg->insn_count; i++ )
    {
        const struct dasm_insn_info *info = &cfg->insns[i].info;
        unsigned int t;

        fprintf( out, "    {\"addr\": %u, \"addr_hex\": \"%04X\", \"next\": %u, \"next_hex\": \"%04X\", \"flow\": \"%s\", \"text\": \"",
                 info->addr, info->addr, info->next, info->next, dasm_cfg_flow_name( info->flow ) );
        json_escape( out, info->text );
        fprintf( out, "\", \"targets\": [" );
        for ( t = 0; t < info->target_count; t++ )
            fprintf( out, "%s{\"addr\": %u, \"addr_hex\": \"%04X\"}",
                     t ? ", " : "", info->targets[t], info->targets[t] );
        fprintf( out, "]}%s\n", i + 1 == cfg->insn_count ? "" : "," );
    }

    fprintf( out, "  ],\n  \"edges\": [\n" );
    for ( i = 0; i < cfg->edge_count; i++ )
    {
        const struct cfg_edge *edge = &cfg->edges[i];

        fprintf( out, "    {\"from\": %u, \"from_hex\": \"%04X\", \"kind\": \"%s\"",
                 edge->from, edge->from, edge_kind_name( edge->kind ) );
        if ( edge->has_to )
            fprintf( out, ", \"to\": %u, \"to_hex\": \"%04X\"", edge->to, edge->to );
        fprintf( out, "}%s\n", i + 1 == cfg->edge_count ? "" : "," );
    }
    fprintf( out, "  ]\n}\n" );
}

const char *cfg_default_extension( const char *format )
{
    if ( !strcmp( format, "dot" ) )
        return "dot";
    if ( !strcmp( format, "json" ) )
        return "json";
    if ( !strcmp( format, "cmd" ) )
        return "cfg";
    return NULL;
}

char *cfg_default_output_path( const char *listfile, const char *format )
{
    const char *slash = strrchr( listfile, '/' );
    const char *base = slash ? slash + 1 : listfile;
    const char *dot = strrchr( base, '.' );
    const char *ext = cfg_default_extension( format );
    char *out;
    size_t prefix_len;
    size_t len;

    if ( !ext )
        error( "Unsupported CFG output format '%s'", format );

    if ( !strcmp( format, "cmd" ) && dot )
    {
        prefix_len = (size_t)(dot - listfile);
        len = prefix_len + strlen(".cfg") + strlen(dot) + 1;
        out = zalloc( len );
        memcpy( out, listfile, prefix_len );
        sprintf( out + prefix_len, ".cfg%s", dot );
        return out;
    }

    prefix_len = dot ? (size_t)(dot - listfile) : strlen( listfile );
    len = prefix_len + 1 + strlen( ext ) + 1;
    out = zalloc( len );
    memcpy( out, listfile, prefix_len );
    sprintf( out + prefix_len, ".%s", ext );
    return out;
}
