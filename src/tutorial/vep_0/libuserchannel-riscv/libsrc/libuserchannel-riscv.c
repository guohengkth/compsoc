#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <platform.h>
#include "cheapout.h"

// we assume that FIFOs of all partitions on all tiles are mapped
// on the same RISC-V memory location (TILE0_AXI_BRAM_CTRL_0_S_AXI)

static uint64_t sent = 0;
static uint64_t received = 0;

uint32_t on_riscv_poll_from_arm_to_riscv (void)
{
  volatile cheapout riscv_stdinout = (cheapout) TILE0_AXI_BRAM_CTRL_0_S_AXI; 
  volatile cheap riscv_stdin = (volatile cheap) &(riscv_stdinout->admin_user_in);
  return cheap_get_tokens_remaining (riscv_stdin);
}

uint32_t on_riscv_poll_from_riscv_to_arm (void)
{
  volatile cheapout riscv_stdinout = (cheapout) TILE0_AXI_BRAM_CTRL_0_S_AXI; 
  volatile cheap riscv_stdout = (volatile cheap) &(riscv_stdinout->admin_user_out);
  return cheap_get_spaces_remaining (riscv_stdout);
}

uint8_t on_riscv_rcv_from_arm_to_riscv (void)
{
  volatile cheapout riscv_stdinout = (cheapout) TILE0_AXI_BRAM_CTRL_0_S_AXI; 
  volatile cheap riscv_stdin = (volatile cheap) &(riscv_stdinout->admin_user_in);
  volatile uint8_t *data[1];
  while (cheap_claim_tokens (riscv_stdin, (volatile void**) data, 1) == 0) ;
  uint8_t c = *data[0];
  cheap_release_spaces(riscv_stdin, 1);
  received++;
  return c;
}

void on_riscv_snd_from_riscv_to_arm (uint8_t c)
{
  volatile cheapout riscv_stdinout = (cheapout) TILE0_AXI_BRAM_CTRL_0_S_AXI; 
  volatile cheap riscv_stdout = (volatile cheap) &(riscv_stdinout->admin_user_out);
  volatile uint8_t *data[1];
  while (cheap_claim_spaces (riscv_stdout, (volatile void **) data, 1) == 0) ;
  *((volatile uint8_t*)data[0]) = c;
  cheap_release_tokens(riscv_stdout, 1);
  sent++;
}

void reset_counters ()
{
  sent = 0;
  received = 0;
}

uint64_t on_riscv_bytes_sent_from_riscv_to_arm ()
{
  return sent;
}

uint64_t on_riscv_bytes_received_from_arm_to_riscv ()
{
  return received;
}
