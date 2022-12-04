#include <stdlib.h>
#include <errno.h>
#include <fcntl.h> //open
#include <unistd.h> //close
#include <sys/mman.h> //mmap
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "cheapout.h"
#include "libuserchannel-arm.h"

static volatile cheap user_stdin[NUM_TILES*NUM_VPS];
static volatile cheap user_stdout[NUM_TILES*NUM_VPS];
static uint64_t bytes_sent_from_arm_to_riscv[NUM_TILES*NUM_VPS];
static uint64_t bytes_received_from_riscv_to_arm[NUM_TILES*NUM_VPS];
static volatile void *ocm[NUM_TILES];
static int active_channels = 0;

// Setup to use normal malloc and free
void * ( *_dynmalloc )( size_t ) = malloc;
void   ( *_dynfree )( void* )    = free;

int on_arm_initialise (int tileid, int pid)
{
  if (tileid < 0 || tileid >= NUM_TILES || pid < 0 || pid > NUM_VPS) {
    fprintf(stderr, "on_arm_initialise: invalid tileid=%d or pid=%d\n",tileid,pid);
    return 1;
  }
  const int index = tileid*NUM_VPS + pid;
  if (user_stdin[index] != NULL) {
    fprintf(stderr, "on_arm_initialise: channel tileid=%d pid=%d already initialised\n",tileid,pid);
    return 0;
  }
  int memf = open("/dev/mem" , O_RDWR | O_SYNC );
  if (memf < 0) {
    int err = errno;
    fprintf(stderr, "FAILED open /dev/mem %d\n", err);
    return err;
  }
  if (ocm[tileid] == NULL) {
    ocm[tileid] = mmap(NULL, OCM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memf, OCM_LOC[tileid]);
    if(ocm[tileid] == MAP_FAILED) {
      int err = errno;
      fprintf(stderr, "FAILED open ocm %d\n", err);
      if (memf > 0) close(memf);
    }
  }

  shared_memory_map *map = (shared_memory_map*)ocm[tileid];
  volatile cheapout admin_v = (cheapout)&(map->vps[pid-1]);//ocm[tileid] + pid*CHEAP_MAX_VP_SIZE);
  user_stdin[index] = (volatile cheap) &(admin_v->admin_user_out);
  user_stdout[index] = (volatile cheap) &(admin_v->admin_user_in);
  reset_counters (tileid, pid);
  active_channels++;

  if (memf > 0) close(memf);
  return 0;
}

void on_arm_cleanup (int tileid, int pid)
{
  if (tileid < 0 || tileid >= NUM_TILES || pid < 0 || pid > NUM_VPS) {
    fprintf(stderr, "on_arm_cleanup: invalid tileid=%d or pid=%d\n",tileid,pid);
    exit(1);
  }
  const int index = tileid*NUM_VPS + pid;
  if (user_stdin[index] == NULL) {
    fprintf(stderr, "on_arm_cleanup: channel tileid=%d pid=%d not initialised\n",tileid,pid);
    exit(1);
  }
  active_channels--;
  // nothing else to do?
}

uint32_t on_arm_poll_from_riscv_to_arm (int tileid, int pid)
{
  const int index = tileid*NUM_VPS + pid;
  if (tileid < 0 || tileid >= NUM_TILES || pid < 0 || pid > NUM_VPS) {
    fprintf(stderr, "on_arm_poll_from_riscv_to_arm: invalid tileid=%d or pid=%d\n",tileid,pid);
    exit(1);
  }
  if (user_stdin[index] == NULL) {
    fprintf(stderr, "on_arm_poll_from_riscv_to_arm: channel tileid=%d pid=%d not initialised; forgot to call on_arm_initialise()?\n",tileid,pid);
    exit(1);
  }
  return cheap_get_tokens_remaining (user_stdin[index]);
}

uint32_t on_arm_poll_from_arm_to_riscv (int tileid, int pid)
{
  const int index = tileid*NUM_VPS + pid;
  if (tileid < 0 || tileid >= NUM_TILES || pid < 0 || pid > NUM_VPS) {
    fprintf(stderr, "on_arm_poll_from_arm_to_riscv: invalid tileid=%d or pid=%d\n",tileid,pid);
    exit(1);
  }
  if (user_stdin[index] == NULL) {
    fprintf(stderr, "on_arm_poll_from_arm_to_riscv: channel tileid=%d pid=%d not initialised; forgot to call on_arm_initialise()?\n",tileid,pid);
    exit(1);
  }
  return cheap_get_spaces_remaining (user_stdout[index]);
}

uint8_t on_arm_rcv_from_riscv_to_arm (int tileid, int pid)
{
  const int index = tileid*NUM_VPS + pid;
  if (tileid < 0 || tileid >= NUM_TILES || pid < 0 || pid > NUM_VPS) {
    fprintf(stderr, "on_arm_rcv_from_riscv_to_arm: invalid tileid=%d or pid=%d\n",tileid,pid);
    exit(1);
  }
  if (user_stdin[index] == NULL) {
    fprintf(stderr, "on_arm_rcv_from_riscv_to_arm: channel tileid=%d pid=%d not initialised; forgot to call on_arm_initialise()?\n",tileid,pid);
    exit(1);
  }
  volatile uint8_t *data[1];
  while (cheap_claim_tokens (user_stdin[index], (volatile void **) data, 1) == 0) ;

  shared_memory_map *map = (shared_memory_map*)ocm[tileid];
  uint32_t offset = ((uint32_t)data[0])-PLATFORM_OCM_MEMORY_LOC + /*pid*CHEAP_MAX_VP_SIZE*/((uint32_t)&(map->vps[pid-1]) - (uint32_t)map);
  uint8_t c = ((volatile uint8_t *)ocm[tileid])[offset];
  cheap_release_spaces(user_stdin[index], 1);
  bytes_received_from_riscv_to_arm[index]++;
  return c;
}

void on_arm_snd_from_arm_to_riscv (int tileid, int pid, uint8_t c)
{
  const int index = tileid*NUM_VPS + pid;
  if (tileid < 0 || tileid >= NUM_TILES || pid < 0 || pid > NUM_VPS) {
    fprintf(stderr, "on_arm_snd_from_arm_to_riscv: invalid tileid=%d or pid=%d\n",tileid,pid);
    exit(1);
  }
  if (user_stdin[index] == NULL) {
    fprintf(stderr, "on_arm_snd_from_arm_to_riscv: channel tileid=%d pid=%d not initialised; forgot to call on_arm_initialise()?\n",tileid,pid);
    exit(1);
  }
  volatile uint8_t *data[1];
  while (cheap_claim_spaces (user_stdout[index], (volatile void **) data, 1) == 0) ;
  //uint32_t offset = ((uint32_t)data[0]) -PLATFORM_OCM_MEMORY_LOC + pid*CHEAP_MAX_VP_SIZE;
  shared_memory_map *map = (shared_memory_map*)ocm[tileid];
  uint32_t offset = ((uint32_t)data[0])-PLATFORM_OCM_MEMORY_LOC + /*pid*CHEAP_MAX_VP_SIZE*/((uint32_t)&(map->vps[pid-1]) - (uint32_t)map);
  ((volatile uint8_t*)ocm[tileid])[offset] = c;
  cheap_release_tokens(user_stdout[index], 1);
  bytes_sent_from_arm_to_riscv[index]++;
}

void reset_counters (int tileid, int pid)
{
  const int index = tileid*NUM_VPS + pid;
  bytes_sent_from_arm_to_riscv[index] = 0;
  bytes_received_from_riscv_to_arm[index] = 0;
}

uint64_t on_arm_bytes_sent_from_arm_to_riscv (int tileid, int pid)
{
  const int index = tileid*NUM_VPS + pid;
  return bytes_sent_from_arm_to_riscv[index];
}

uint64_t on_arm_bytes_received_from_riscv_to_arm (int tileid, int pid)
{
  const int index = tileid*NUM_VPS + pid;
  return bytes_received_from_riscv_to_arm[index];
}
