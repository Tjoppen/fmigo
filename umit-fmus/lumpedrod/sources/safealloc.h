#ifndef SAFE_ALLOC_H
#define SAFE_ALLOC_H
#define MALLOC(T, x, n) ((x) = ( T * ) malloc(sizeof( *x ) * ( n ) ), assert(x))
#define CALLOC(T, x, n) ((x) = ( T * ) calloc(n, sizeof( *x ) ), assert(x))
#define FREE( x ) ( assert( x ), free( x ) )
#endif
