#include <xil_printf.h>
#include <memmap.h>
#include <cheap_s.h>

#define FIFODEPTH 4
typedef struct {
  int32_t your_data; // or whatever you want
} token_t;

int main ( void )
{
  // volatile const T * const: can read but not modify the timer
  volatile const uint64_t * const g_timer = (uint64_t*)TILE0_GLOBAL_TIMER;

  volatile cheap const fifo_admin = (cheap) tile1_comm0;
  volatile token_t * const fifo_ptr;
  // wait until the C-HEAP FIFO has been initialised
  while (cheap_get_buffer_capacity (fifo_admin) == 0) ; 
  uint64_t t = *g_timer;
  xil_printf("%04u/%010u: C-HEAP FIFO has been initialised, admin=0x%X\n",
      (uint32_t)(t>>32),(uint32_t)t, (uint32_t) fifo_admin);

  int cnt = 0;
  uint64_t start = *g_timer;
  do {
    while (cheap_claim_tokens (fifo_admin, (volatile void **) &fifo_ptr, 1) == 0) ;
    // use the token ... fifo_ptr->your_data
    if (++cnt % 50000 == 0) {
      uint64_t t = *g_timer;
      xil_printf("%04u/%010u: %d tokens (%u tokens/sec)\n",
	  (uint32_t)(t>>32),(uint32_t)t, cnt, (uint32_t)(((uint64_t)cnt*40e6)/(t - start)));
    }
    cheap_release_spaces (fifo_admin, 1);
  } while (1);
  return 0;
}
