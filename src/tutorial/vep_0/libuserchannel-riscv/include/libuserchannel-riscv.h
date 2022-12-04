#ifndef _LIBCHANNELRISCV
#define _LIBCHANNELRISCV

#include <stdint.h>

// we assume that FIFOs of all partitions on all tiles are mapped
// on the same RISC-V memory location (TILE0_AXI_BRAM_CTRL_0_S_AXI)

// is there data from the arm?
extern uint32_t on_riscv_poll_from_arm_to_riscv (void);
// is there space to send to the arm?
extern uint32_t on_riscv_poll_from_riscv_to_arm (void);
extern uint8_t on_riscv_rcv_from_arm_to_riscv (void);
extern void on_riscv_snd_from_riscv_to_arm (uint8_t c);
extern void reset_counters ();
extern uint64_t on_riscv_bytes_sent_from_riscv_to_arm ();
extern uint64_t on_riscv_bytes_received_from_arm_to_riscv ();

#endif
