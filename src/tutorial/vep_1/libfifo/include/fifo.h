#ifndef _FIFO_H_
#define _FIFO_H_

#include <stdint.h>

#ifndef TOKEN_DATA
#define TOKEN_DATA 6
#endif
typedef struct _token_t {
  int32_t data[TOKEN_DATA];
  uint64_t timestamp;
} token_t;

#ifndef FIFO_DEPTH
#define FIFO_DEPTH 10
#endif
typedef volatile struct _fifo_t {
  token_t token[FIFO_DEPTH];
  // your magic
} fifo_t;

extern void fifo_status (volatile const fifo_t * const fifo);

// the number of spaces in the FIFO
// (including those claimed but not yet released)
extern uint32_t spaces (volatile const fifo_t * const fifo);

// the number of tokens in the FIFO
// (including those claimed but not yet released)
extern uint32_t tokens (volatile const fifo_t * const fifo);
// note that spaces+tokens is equal to the FIFO capacity

// non-blocking, return NULL when no space available
extern volatile token_t * poll_space (volatile fifo_t * const fifo);

// blocking, wait until space is available
// it is not possible to claim multiple spaces before releasing tokens
extern volatile token_t * claim_space (volatile fifo_t * const fifo);

// return 0 upon success, 1 upon failure (like releasing token when no space has been claimed)
extern uint32_t release_token (volatile fifo_t * const fifo);

// non-blocking, return NULL when no token available
extern volatile token_t * poll_token (volatile fifo_t * const fifo);

// blocking, wait until token is available
// it is not possible to claim multiple tokens before releasing spaces
extern volatile token_t * claim_token (volatile fifo_t * const fifo);

// return 0 upon success, 1 upon failure (like releasing space when no token has been claimed)
extern uint32_t release_space (volatile fifo_t * const fifo);

#endif
