#include "vep_malloc.h"
#include "xil_printf.h"

// create a new MMCB on the current heap
// this uses the standard higher-level malloc function
MMCB *vep_malloc_create(unsigned int *start, unsigned int *end)
{
  MMCB *retv = malloc(sizeof(*retv));
  // setup safe values
  retv->heap_start = start;
  retv->heap_end = end;
  // indicate not yet initialized
  retv->initialized = 0;
  return retv;
}

/**
 * The first time a mk_malloc is executed, this function will be
 * called to initialize the administration. A not-used MCB is
 * created at the start of the heap, and the initialization flag is set.
 */
static void vep_init_malloc(MMCB *mmcb)
{
  int* mcb;

  // write an not-used MCB at the start of the heap
  mcb = mmcb->heap_start;
  ((MCB*)mcb)->is_used = 0;
  ((MCB*)mcb)->wsize = (mmcb->heap_end - mmcb->heap_start) - sizeof(MCB)/4;
  ((MCB*)mcb)->parent = mmcb;
  // do the administration
  mmcb->last_allocated_mcb = mcb;
  mmcb->heap_max = 0;
  mmcb->heap_usage = 0;
  mmcb->num_mallocs = 0;
  mmcb->num_frees = 0;
  mmcb->initialized = 1;
}

void vep_free(MMCB *mmcb, void *p)
{
  if (mmcb != ((MCB*)(p-sizeof(MCB)))->parent) {
    xil_printf("vep_free: error: invalid memory %p, was not allocated.\r\n", p);
    return;
  }
  if (!mmcb->initialized) return;
  if (p != NULL)
  {
    ((MCB*)(p-sizeof(MCB)))->is_used = 0;
    mmcb->heap_usage -= (((MCB*)(p-sizeof(MCB)))->wsize + sizeof(MCB)/4);
    mmcb->num_frees++;
  }
}

void* vep_malloc(MMCB *mmcb, size_t size)
{
  void* ptr;
  int word_size;
  int* mcb;
  int skip_incr = 0;

  if (mmcb->heap_start == NULL || mmcb->heap_end == NULL || mmcb->heap_start >= mmcb->heap_end) {
    xil_printf("vep_malloc: warning: out of memory\r\n");
    return NULL;
  }

  // check size for 0
  if (size == 0) return NULL;

  if (mmcb->initialized == 0) {
    vep_init_malloc(mmcb);
  }

  //create proper wordsize (round off upward)
  word_size = size>>2;
  if (size % 4 != 0) word_size++;

  int* start_mcb = mmcb->last_allocated_mcb;
  int went_from_end_to_start = 0;
  mcb = NULL;

  // first try to find a spot in the heap
  // the searching starts at last_allocated_mcb, goes up, goes to start when end has reached,
  // then goes up again until start_point is reached
  while(!went_from_end_to_start || mmcb->last_allocated_mcb < start_mcb ) {
    //check if current is not used
    if (!(((MCB*)mmcb->last_allocated_mcb)->is_used)) {
      // check if the current mcb has enough space
      if ( ((MCB*)mmcb->last_allocated_mcb)->wsize >= (int)word_size ) {
        // we have found a place in the heap, so set mcb
        mcb = mmcb->last_allocated_mcb;
        //check if we have to split the current mcb
        int left_over = ((MCB*)mcb)->wsize - (word_size);
        if (left_over > (int) sizeof(MCB)/4) {
          // we have to split the current mcb
          int* next_mcb = mcb + ((MCB*)mcb)->wsize + sizeof(MCB)/4;
          int* new_middle_mcb = mcb + word_size + sizeof(MCB)/4;
          // work backward and set all mcb_values

          ((MCB*)new_middle_mcb)->is_used = 0;
          ((MCB*)new_middle_mcb)->wsize = left_over - sizeof(MCB)/4;
          ((MCB*)new_middle_mcb)->parent = mmcb;

          ((MCB*)mcb)->is_used = 1;
          ((MCB*)mcb)->wsize = word_size;
          ((MCB*)mcb)->parent = mmcb;
        } else {
          // we do not split the current mcb
          ((MCB*)mcb)->is_used = 1;
          ((MCB*)mcb)->parent = mmcb;
          // we do not change the size here, although it might be slightly larger than word_size
        }
        // return
        break;
      } else {  // the current MCB is not used, but is too small
        // in this case we try to combine the current MCB with the next MCB
        int* next_mcb = mmcb->last_allocated_mcb + ((MCB*)mmcb->last_allocated_mcb)->wsize + sizeof(MCB)/4;
        if (next_mcb < mmcb->heap_end) { //we don't want to mess with the stack
          // check if next mcb is used
          if (!(((MCB*)next_mcb)->is_used)) {
            // now merge the two MCBs
            ((MCB*)mmcb->last_allocated_mcb)->wsize += ((MCB*)next_mcb)->wsize + sizeof(MCB)/4;
            // update next_mcb pointer to the new next mcb
            next_mcb = mmcb->last_allocated_mcb + ((MCB*)mmcb->last_allocated_mcb)->wsize + sizeof(MCB)/4;
            // set skip_increment
            skip_incr = 1;
          }
        } else {
          /**
           * The next MCB block lies beyond the end of the heap.
           * Validate it is end, and we did not overshoot.
           */
          if (next_mcb != mmcb->heap_end)  {
            // We overshot the end of the heap, this should not happen.
            // This is a fatal error, abort execution of this partition.
            xil_printf("vep_malloc: internal error: memory allocation overflow.\r\n");
            return NULL;
          }
          /**
           * We are at the end of the heap and there is insufficient space at the end of the heap.
           * We are not overflowed, therefor we will either return to start, or quit if we already
           * went round once.
           */
        }
      }
    }
    // increase current mcb (unless skip_incr is set)
    if (!skip_incr) {
      // compute next mcb
      mmcb->last_allocated_mcb = mmcb->last_allocated_mcb + ((MCB*)mmcb->last_allocated_mcb)->wsize + sizeof(MCB)/4;
      // in case we have reached the end of the heap, we go back to the start
      if (mmcb->last_allocated_mcb >= mmcb->heap_end) {
        mmcb->last_allocated_mcb = mmcb->heap_start;
        went_from_end_to_start = 1;
      }
    } else {
      // reset skip_incr
      skip_incr = 0;
    }
  }

  // determine the return value
  if (mcb != NULL) {
    ptr = (void*)(mcb + sizeof(MCB)/4);

    mmcb->heap_usage += sizeof(MCB)/4+((MCB *)mcb)->wsize;
    if (mmcb->heap_usage > mmcb->heap_max) {
      mmcb->heap_max = mmcb->heap_usage;
    }
    mmcb->num_mallocs++;
  } else {
    ptr = NULL;
  }

  if (ptr == NULL) {
    xil_printf("vep_malloc: warning: out of memory\r\n");
  }

  return ptr;
}

uint32_t vep_malloc_get_size(const MMCB *mmcb)
{
  return (mmcb->heap_end - mmcb->heap_start);
}

uint32_t vep_malloc_usage(const MMCB *mmcb)
{
  if (!mmcb->initialized) return 0;
  return mmcb->heap_usage;
}

uint32_t vep_malloc_max_usage(const MMCB *mmcb)
{
  if (!mmcb->initialized) return 0;
  return mmcb->heap_max;
}

uint32_t vep_malloc_num_mallocs(const MMCB *mmcb)
{
  if (!mmcb->initialized) return 0;
  return mmcb->num_mallocs;
}

uint32_t vep_malloc_num_frees(const MMCB *mmcb)
{
  if (!mmcb->initialized) return 0;
  return mmcb->num_frees;
}

