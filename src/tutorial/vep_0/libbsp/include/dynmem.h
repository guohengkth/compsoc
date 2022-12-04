#ifndef LIBCHANNEL_DYNMEM_H
#define LIBCHANNEL_DYNMEM_H

#include <stddef.h>

extern void * ( *_dynmalloc )(size_t size);
extern void   ( *_dynfree )( void* ptr );

#endif
