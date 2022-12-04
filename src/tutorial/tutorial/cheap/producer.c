#include <xil_printf.h>
#include <memmap.h>
#include <cheap_s.h>

#define FIFODEPTH 10
typedef struct {
  int32_t data[6];    // the data
  uint64_t timestamp; // indicates when the data was produced
} token_t;

int main ( void )
{
  // volatile const T * const: can read but not modify the timer
  volatile const uint64_t * const g_timer = (uint64_t*)TILE0_TIMER_0_S_AXI;
  uint64_t t;

  /*
   * 1. fifo_admin: defines where the C-HEAP FIFO administration (type cheap) is located in memory
   * 2. fifo_data: define where the actual FIFO data (pointer token_t) is located in memory
   *    note that the +1 points to the first memory location after fifo_admin
   * 3. fifo_ptrs: array of pointers to store pointers to the space/tokens you asked for;
   *    it should be able to contain at least as many empty tokens as you ask for,
   *    but it is good practice to declare it with length FIFODEPTH
   * 4. at the producer side: initialise fifo_admin
   *    the function returns NULL on failure (and its first argument on success)
   * 5. note that because the shared memories are zero initialised,
   *    if you use an C-HEAP FIFO administration that is uninitialised,
   *    it will be interpreted as an empty FIFO (read=write=0 => empty)
   */
  volatile cheap const fifo_admin = (cheap) tile0_comm1;
  volatile token_t * const fifo_data = (token_t *)(fifo_admin +1);
  volatile token_t *fifo_ptrs[FIFODEPTH];
  t = *g_timer;
  // initialise the C-HEAP FIFO
  if (cheap_init_r (fifo_admin, (void *) fifo_data, FIFODEPTH, sizeof(token_t)) == NULL) {
    xil_printf("%04u/%010u: cheap_init_r failed\n", (uint32_t)(t>>32),(uint32_t)t);
    return 1;
  }
  xil_printf("%04u/%010u: C-HEAP FIFO has been initialised, admin=0x%X data=0x%X ptrs=0x%X\n",
      (uint32_t)(t>>32),(uint32_t)t, (uint32_t) fifo_admin, (uint32_t) fifo_data, (uint32_t) fifo_ptrs);

  int32_t cnt = 0;
  // for demonstration purposes we vary the number of tokens we claim
  int claim = 1;
  do {
    /*
     * 1. call claim_spaces, which returns the number of empty tokens you got
     *    if it's less than we asked we return them and try again
     * 2. each fifo_ptrs[i] is a pointer to an empty token,
     *    and you can put your data there (fifo_ptrs[i]->... = ...)
     */
    int nr_tokens;
    // claim spaces, return them if it's less than we want
    while ((nr_tokens = cheap_claim_spaces (fifo_admin, (volatile void **) &fifo_ptrs[0], claim)) < claim)
      cheap_release_all_claimed_spaces (fifo_admin);
    xil_printf("%04u/%010u: claim_spaces %d\n", (uint32_t)(t>>32), (uint32_t)t, nr_tokens);

    // fill the tokens
    for (int i=0; i < nr_tokens; i++) {
      for (int d = 0; d < 6; d++) fifo_ptrs[i]->data[d] = 10*cnt+d;
      t = *g_timer;
      fifo_ptrs[i]->timestamp = *g_timer;
      xil_printf("%04u/%010u: space[%d]=0x%08X\n",
	  (uint32_t)(t>>32),(uint32_t)t, i, (uint32_t)fifo_ptrs[i]);
      xil_printf("%04u/%010u: write token[%d]=0x%08X ts=%010u data=%d,%d,%d,%d,%d,%d\n",
	  (uint32_t)(t>>32), (uint32_t)t, i, (uint32_t) fifo_ptrs[i], (uint32_t) fifo_ptrs[i]->timestamp,
	  fifo_ptrs[i]->data[0], fifo_ptrs[i]->data[1], fifo_ptrs[i]->data[2],
	  fifo_ptrs[i]->data[3], fifo_ptrs[i]->data[4], fifo_ptrs[i]->data[5]);
      cnt++;
    }
    cheap_release_tokens (fifo_admin, nr_tokens);
    claim = (claim >= FIFODEPTH/2 ? 1 : claim +1);
    t = *g_timer;
    xil_printf("%04u/%010u: release_tokens %d\n", (uint32_t)(t>>32), (uint32_t)t, nr_tokens);
  } while (1);
  return 0;
}
