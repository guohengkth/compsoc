#ifndef _VEP_MALLOC_H_
#define _VEP_MALLOC_H_

#include <stdint.h>
#include <stdlib.h>

// high level Memory Management Control Block
typedef struct _MMCB MMCB;
typedef struct MCB_s MCB;

// control block for dynamic memory allocation
struct _MMCB {
  int *heap_start;
  int *heap_end;

  int32_t  initialized;
  // variables for heap management
  int *last_allocated_mcb;

  // statistics
  int32_t  heap_usage;
  int32_t  heap_max;
  int32_t  num_mallocs;
  int32_t  num_frees;
};

// struct for dynamic memory allocation
struct MCB_s {
  int is_used;        // indicates if the block is used
  int wsize;          // size of this block in words
  MMCB *parent;
};

// create a new heap, with start & end addresses of the heap
// returns a new initialized MMCB
MMCB *vep_malloc_create(unsigned int *start,unsigned int *end) __attribute__((nonnull));

// allocate a memory block, size of the memory to be allocated in bytes
// return pointer to the allocated memory, NULL if it fails.
void* vep_malloc(MMCB *mmcb, size_t size) __attribute__((malloc,nonnull(1)));

// function to free a memory location.
// p pointer to the location to be freed.
void vep_free(MMCB *mmcb, void* p) __attribute__((nonnull(1)));


// statistics

// return heap size in words
uint32_t vep_malloc_get_size(const MMCB *mmcb);

// return the current heap usage in words
// due to fragmenation, allocation can fail even when there's enough space left
uint32_t vep_malloc_usage(const MMCB *mmcb) __attribute__((nonnull(1)));

// return the maximum heap usage in words
uint32_t vep_malloc_max_usage(const MMCB *mmcb) __attribute__((nonnull(1)));

// return the number of mallocs
uint32_t vep_malloc_num_mallocs(const MMCB *mmcb);

// return the number of frees
uint32_t vep_malloc_num_frees(const MMCB *mmcb);

#endif
