#ifndef TIMERS_H
#define TIMERS_H

#include <stdint.h>
#include <platform.h>

// read the 64-bit global or partition timer, ensuring consistent 32-bit words
// WARNING: this does NOT work with -Os compile option (which probably turns off inline)

#define read_global_timer _read_global_timer
inline uint64_t _read_global_timer()
{
  volatile const uint64_t * const timer = (uint64_t *) TILE0_GLOBAL_TIMER;
  volatile const uint32_t * const timer_ls = (uint32_t *) TILE0_GLOBAL_TIMER;
  uint64_t t1;
  uint32_t t2_ls;
  while (1) {
    t1 = *timer;
    t2_ls = *timer_ls;
    if (t2_ls >= ((uint32_t) t1) + 12 && t2_ls <= ((uint32_t) t1) + 18) return t1;
  }
}

#define read_partition_timer _read_partition_timer
inline uint64_t _read_partition_timer()
{
  volatile const uint64_t * const timer = (uint64_t *) TILE0_PARTITION_TIMER;
  volatile const uint32_t * const timer_ls = (uint32_t *) TILE0_PARTITION_TIMER;
  uint64_t t1;
  uint32_t t2_ls;
  while (1) {
    t1 = *timer;
    t2_ls = *timer_ls;
    if (t2_ls >= ((uint32_t) t1) + 12 && t2_ls <= ((uint32_t) t1) + 18) return t1;
  }
}

#endif
