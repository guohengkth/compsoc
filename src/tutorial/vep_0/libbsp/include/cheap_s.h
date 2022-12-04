#ifndef LIBCHANNEL_CHEAP_S_H
#define LIBCHANNEL_CHEAP_S_H

#include <cheap.h>

/** c-heap admin */
struct cheap_s
{
    /** Size of the token (in bytes) */
    uint32_t         token_size;

    /** Capacity of the buffer (in tokens) */
    uint32_t         buffer_capacity;

    /** The local buffer */
    volatile void *  buffer;

    /** If the buffer is malloced */
    uint32_t         malloced_buffer;

    /** The write counter */
    volatile int32_t writec;

    /** The read counter */
    volatile int32_t readc;

    /** The number of claimed spaces */
    uint32_t         claimed_spaces;

    /** The number of claimed tokens */
    uint32_t         claimed_tokens;
};

#endif //LIBCHANNEL_CHEAP_S_H

