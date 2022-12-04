#include <xil_printf.h>
#include <memmap.h>
#include <cheap_s.h> /* NOT: cheap.h */

#define FIFODEPTH 10
typedef struct {
  int32_t data[6];    // the data
  uint64_t timestamp; // indicates when the data was produced
} token_t;

int main ( void )
{
  // volatile const T * const: can read but not modify the timer
  volatile const uint64_t * const g_timer = (uint64_t*)TILE0_GLOBAL_TIMER;
  uint64_t t;

  /*
   * 1. define where the C-HEAP FIFO administration is located in memory
   * 2. at the consumer side: wait until the FIFO is initialised by polling on the FIFO capacity
   *    (this works because the shared memories are zero initialised)
   *    if you use an C-HEAP FIFO administration that is uninitialised,
   *    it will be interpreted as an empty FIFO (d=write=0 => empty)
   */
  volatile cheap const fifo_admin = (cheap) tile1_comm0;
  // not really needed here, but useful for memory layout when using multiple FIFOs
  volatile token_t * const fifo_data = (token_t *)(fifo_admin +1);
  volatile token_t *fifo_ptrs[FIFODEPTH];
  // wait until the C-HEAP FIFO has been initialised
  while (cheap_get_buffer_capacity (fifo_admin) == 0) ; 
  t = *g_timer;
  xil_printf("%04u/%010u: C-HEAP FIFO has been initialised, admin=0x%X data=0x%X ptrs=0x%X\n",
      (uint32_t)(t>>32),(uint32_t)t, (uint32_t) fifo_admin, (uint32_t) fifo_data, (uint32_t) fifo_ptrs);

  int32_t cnt = 0;
  // for demonstration purposes we vary the number of tokens we claim
  int claim = FIFODEPTH/2; 
  do {
    /*
     * 1. call claim_tokens, which returns the number of tokens you got
     *    if it's less than we asked we return them and try again
     * 2. each fifo_ptrs[i] is a pointer to an full token, and you can get your data there
     */
    int nr_tokens;
    // to wait until you have ALL requested tokens:
    while ((nr_tokens = cheap_claim_tokens (fifo_admin, (volatile void **) fifo_ptrs, claim)) < claim)
      cheap_release_all_claimed_tokens (fifo_admin);
    // alternatively, wait until you have at least one token:
    // nr_tokens = cheap_claim_tokens (fifo_admin, (volatile void **) fifo_ptrs, claim);
    // if (nr_tokens == 0) continue;

    t = *g_timer;
    uint64_t d1 = (uint64_t)t - fifo_ptrs[0]->timestamp;
    uint64_t d2 = (uint64_t)t - fifo_ptrs[nr_tokens-1]->timestamp;
    xil_printf("%04u/%010u: claim_tokens %d (first %d,%u; last %d,%u) with delays %u-%u\n",
	(uint32_t)(t>>32), (uint32_t)t, nr_tokens,
	fifo_ptrs[0]->data[0], (uint32_t)fifo_ptrs[0]->timestamp, 
	fifo_ptrs[nr_tokens-1]->data[0], (uint32_t)fifo_ptrs[nr_tokens-1]->timestamp,
	(uint32_t) d1, (uint32_t) d2);

    // read the tokens
    for (int i=0; i < nr_tokens; i++) {
      xil_printf("%04u/%010u: read token[%d]=0x%08X ts=%010u data=%d,%d,%d,%d,%d,%d\n",
	  (uint32_t)(t>>32), (uint32_t)t, i, (uint32_t) fifo_ptrs[i], (uint32_t) fifo_ptrs[i]->timestamp,
	  fifo_ptrs[i]->data[0], fifo_ptrs[i]->data[1], fifo_ptrs[i]->data[2],
	  fifo_ptrs[i]->data[3], fifo_ptrs[i]->data[4], fifo_ptrs[i]->data[5]);
      for (int d = 0; d < 6; d++) {
	if (1 && fifo_ptrs[i]->data[d] != 10*cnt+d) {
	  t = *g_timer;
	  xil_printf("%04u/%010u: inconsistent token %d\n", (uint32_t)(t>>32),(uint32_t)t, i);
	  break;
	}
      }
      cnt++;
    }
    cheap_release_spaces (fifo_admin, nr_tokens);
    claim = (claim -1 <= 0 ? FIFODEPTH/2 : claim -1);
    t = *g_timer;
    xil_printf("%04u/%010u: release_spaces %d\n", (uint32_t)(t>>32), (uint32_t)t, nr_tokens);
  } while (1);
  return 0;
}
