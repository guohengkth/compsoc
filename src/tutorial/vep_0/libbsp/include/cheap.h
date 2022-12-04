#ifndef LIBCHANNEL_CHEAP_H
#define LIBCHANNEL_CHEAP_H

#include <stdint.h>

typedef struct cheap_s   cheap_t;
typedef cheap_t*         cheap;

cheap cheap_init ( void * buffer, uint32_t buffer_capacity, uint32_t token_size );

cheap cheap_init_r ( cheap admin, void * buffer, uint32_t buffer_capacity, uint32_t token_size );

void cheap_destroy ( cheap admin );

uint32_t cheap_claim_tokens ( cheap admin, volatile void** token_ptrs, uint32_t tokens );

uint32_t cheap_claim_spaces ( cheap admin, volatile void** space_ptrs, uint32_t spaces );

void cheap_release_tokens ( cheap admin, uint32_t tokens );

void cheap_release_spaces ( cheap admin, uint32_t spaces );

void cheap_release_all_claimed_tokens ( cheap admin );

void cheap_release_all_claimed_spaces ( cheap admin );

uint32_t cheap_get_tokens_remaining ( cheap admin );

uint32_t cheap_get_spaces_remaining ( cheap admin );

uint32_t cheap_get_buffer_capacity ( cheap admin );

#endif //LIBCHANNEL_CHEAP_H

