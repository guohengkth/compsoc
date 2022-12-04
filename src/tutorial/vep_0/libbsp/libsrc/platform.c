#include "platform.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// list of all shared memories
const char shared_mems[NUM_SHARED_MEMORIES][10] = { "mem01", "mem02", "mem12" };
// list of memories reachable from a tile at a base address (-1 if not)
const int shared_mems_start[NUM_TILES][NUM_SHARED_MEMORIES] = 
  { { TILE0_MEM01_START, TILE0_MEM02_START, -1 },
    { TILE1_MEM01_START, -1, TILE1_MEM12_START },
    { -1, TILE2_MEM02_START, TILE2_MEM12_START } };

int get_shared_mem_index(const char * const mem)
{
  if (mem == NULL) return -1;
  for (int i=0; i < NUM_SHARED_MEMORIES; i++)
    if (!strcmp(shared_mems[i], mem)) return i;
  return -1;
}

// get the starting address of the shared memory when accessing from tile t
// returns -1 if the memory doesn't exist or isn't reachable from that tile
int get_shared_mem_start(const char * const mem, const int tile)
{
  if (mem == NULL || tile < 0 || tile >= NUM_TILES) return -1;
  for (int i=0; i < NUM_SHARED_MEMORIES; i++)
    if (!strcmp(shared_mems[i], mem)) return shared_mems_start[i][0];
  return -1;
}

