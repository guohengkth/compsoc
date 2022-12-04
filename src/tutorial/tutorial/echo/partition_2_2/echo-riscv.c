#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <xil_printf.h>
#include <platform.h>
#include <libuserchannel-riscv.h>

int main (void)
{
  // volatile const T * const: can read but not modify the timer
  volatile const uint64_t * const g_timer = (uint64_t*)TILE2_GLOBAL_TIMER;
  uint64_t t = *g_timer;
  xil_printf("%04u/%010u: echo mode:\n", (uint32_t)(t>>32), (uint32_t)t);
  while(1) {
    // read char from user FIFO from ARM & echo on user FIFO to ARM
    uint8_t c = on_riscv_rcv_from_arm_to_riscv();
    on_riscv_snd_from_riscv_to_arm(c);
    t = *g_timer;
    xil_printf("%04u/%010u: echo back \'%c\'\n", (uint32_t)(t>>32), (uint32_t)t, c);
  }
  return 0;
}
