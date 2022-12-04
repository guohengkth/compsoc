#include <xil_printf.h>
#include <memmap.h>
#include <cheap_s.h>

#define FIFODEPTH 4
typedef struct {
  int32_t your_data; // or whatevery you want
} token_t;

int main ( void )
{
  // volatile const T * const: can read but not modify the timer
  volatile const uint64_t * const g_timer = (uint64_t*)TILE0_TIMER_0_S_AXI;

  volatile cheap const fifo_admin = (cheap) tile0_comm1;
  volatile token_t * const fifo_ptr = (token_t *)(fifo_admin +1);
  // initialise the C-HEAP FIFO
  if (cheap_init_r (fifo_admin, (void *) fifo_ptr, FIFODEPTH, sizeof(token_t)) == NULL) {
    uint64_t t = *g_timer;
    xil_printf("%04u/%010u: cheap_init_r failed\n", (uint32_t)(t>>32),(uint32_t)t);
    return 1;
  }

  int cnt = 0;
  do {
    while (cheap_claim_spaces (fifo_admin, (volatile void **) &fifo_ptr, 1) == 0) ; 
    // fill the token: fifo_ptr->mydata = ...
    cheap_release_tokens (fifo_admin, 1);
  } while (1);
  return 0;
}
