#ifndef _ARM_USER_FIFO_LIBH
#define _ARM_USER_FIFO_LIBH

#include <stdint.h>

extern int on_arm_initialise (int tileid, int pid);
extern void on_arm_cleanup (int tileid, int pid);
// is there data from the risc?
extern uint32_t on_arm_poll_from_riscv_to_arm (int tileid, int pid);
// is there space to send to the riscv?
extern uint32_t on_arm_poll_from_arm_to_riscv (int tileid, int pid);
extern uint8_t on_arm_rcv_from_riscv_to_arm (int tileid, int pid);
extern void on_arm_snd_from_arm_to_riscv (int tileid, int pid, uint8_t c);
extern void reset_counters (int tileid, int pid);
extern uint64_t on_arm_bytes_sent_from_arm_to_riscv (int tileid, int pid);
extern uint64_t on_arm_bytes_received_from_riscv_to_arm (int tileid, int pid);

#endif
