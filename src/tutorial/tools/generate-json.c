#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <platform.h>
#include <cheapout.h>

// size of input file
#define MAXLINES 500
#define LINELENGTH 150


int power2(unsigned int i)
{
  unsigned int m = 1;
  while (m && i > m) m = m << 1;
  return (i == m);
}

int my_get_shared_mem_index(const char * const mem)
{
  if (mem == NULL) return -1;
  for (int i=0; i < NUM_SHARED_MEMORIES; i++)
    if (!strcmp(shared_mems[i], mem)) return i;
  return -1;
}

#define USAGE \
    fprintf(stderr, "Usage: %s [-d] <path-to-vep-config.txt> [-ld vep tile partition | -json [-mm] [-tdm] [suspend vep tile partition ...]]\n" , pgm);

/* WARNING
 * NUM_PARTITIONS is used differently from other NUM_* constants
 * NUM_PARTITIONS is 8, meaning that there are 8 user partitions numbered from 1 to 9
 * PARTITION 0 is the system partition, and it must always be present in TDM slot 1 and @ 0 in instr/data memory
 * map->vps[] only contains user partitions
 */

int main(int argc , char *argv[]) {
  const char * const pgm = argv[0];
  if (argc < 2) {
    USAGE;
    return 1;
  }
  int debug = 0;
  int printmm = 0;
  int printtdm = 0;
  int json = 0;
  int ls_vep = -1, ls_tile = -1, ls_partition = -1;
  argc--;
  argv++;
  if (argc > 1 && !strcmp(argv[0], "-d")) {
    debug = 1;
    argc--;
    argv++;
  }
  const char * const file = argv[0];
  argc--;
  argv++;
  if (argc >= 4 && !strcmp(argv[0], "-ld")) {
    // generate lscript for this partition
    ls_vep = atoi(argv[1]);
    ls_tile = atoi(argv[2]);
    ls_partition = atoi(argv[3]);
    if (ls_vep < 1 || ls_tile < 0 || ls_tile >= NUM_TILES || ls_partition < 1 || ls_partition > NUM_PARTITIONS) {
      USAGE;
      return 1;
    }
    argc -= 4;
    argv += 4;
  }
  // should add a "suspend vep" command, -sp / -sv
  if (argc > 0 && !strcmp(argv[0], "-json")) {
    json = 1;
    argc--;
    argv++;
  }
  if (argc > 0 && !strcmp(argv[0], "-mm")) {
    printmm = 1;
    argc--;
    argv++;
  }
  if (argc > 0 && !strcmp(argv[0], "-tdm")) {
    printtdm = 1;
    argc--;
    argv++;
  }
  // next argument, if any must be "suspend"; it's parsed later on
  if (argc > 0 && strcmp(argv[0], "suspend")) {
    USAGE;
    return 1;
  }

  if ((json && ls_vep != -1) || (!json && ls_vep == -1)) {
    fprintf(stderr, "%s: error: must have either -ld or -json option\n", pgm);
    USAGE;
    return 1;
  }

  FILE *vc = fopen(file, "r");
  if (vc == NULL) {
    fprintf(stderr, "%s: error: cannot open %s\n" , pgm, file);
    return 1;
  }

  // read file into array, removing comments & extra white space
  char lines[MAXLINES][LINELENGTH] = { { 0 }, };
  int line = 0, ch = 0;
  int skip = 0;
  while (!feof(vc) && line < MAXLINES && ch < LINELENGTH-1) {
    lines[line][ch] = fgetc(vc);
    // skip comments
    if (lines[line][ch] == '#') { skip = 1; continue; }
    if (skip) { if (lines[line][ch] == '\n') skip = 0; else continue; }
    // merge white space
    if ((ch > 0 && isspace(lines[line][ch-1])) && isspace(lines[line][ch]) && lines[line][ch] != '\n') continue;
    // skip leading spaces
    if (ch == 0 && isspace(lines[line][ch])) continue;
    if (lines[line][ch++] == '\n') { lines[line][ch] = '\0'; line++; ch = 0; }
  }
  fclose(vc);

  if (0) {
    for (int i=0; i < line; i++) {
      fprintf(stderr,"%s", &lines[i][0]);
    }
  }

  unsigned int slot[NUM_TILES] = { 0 }; // nr slots in use on tile
  unsigned int in_vep[NUM_TILES][NUM_PARTITIONS+1] = { 0 };
  unsigned int alloc_memory[NUM_TILES][NUM_PARTITIONS+1] = { 0 };
  unsigned int alloc_memory_start[NUM_TILES][NUM_PARTITIONS+1] = { 0 };
  unsigned int alloc_stack[NUM_TILES][NUM_PARTITIONS+1] = { 0 };
  unsigned int alloc_slot[NUM_TILES][NUM_SLOTS+1] = { 0 };
  unsigned int alloc_slot_start[NUM_TILES][NUM_SLOTS+1] = { 0 };
  unsigned int alloc_slot_length[NUM_TILES][NUM_SLOTS+1] = { 0 };
  unsigned int alloc_period[NUM_TILES] = { 0 };
#define PRIVATE 0
#define PUBLIC 1
  unsigned int alloc_shmem[NUM_SHARED_MEMORIES][2][NUM_VEPS] = { 0 };
  unsigned int alloc_shmem_start[NUM_SHARED_MEMORIES][2][NUM_VEPS] = { 0 };

  // allocate resources for the system application
  for (int t=0; t < NUM_TILES; t++) {
    slot[t] = 1;
    alloc_memory[t][0] = SYS_APP_MEMORY;
    alloc_memory_start[t][0] = 0;
    alloc_stack[t][0] = 1; // not used, but must be > 0
    alloc_slot[t][0] = 0;
    alloc_slot_start[t][0] = 0; // assume that we start with an application slot, followed by a VKERNEL slot
    alloc_slot_length[t][0] = SYS_APP_CYCLES;
  }

  int error = 0, slot_list = -1;
  for (int i=0; i < line; i++) {
    unsigned int tile = 0, partition = 0, stack = DEFAULT_APP_STACK, cycles = DEFAULT_APP_SLOT_CYCLES, memory = 0, start = 0, vep = 0, period = 0, vp = 0;
    int check_mem = 0, check_shared_mem = 0, check_slot_list = 0, check_slot_abs = 0, read = 0, mem_index = -1, public = 0;
    char mem_string[100] = { '\0', };
    if (((read = sscanf(&lines[i][0], "vep %u in %s have %uK public memory starting at %uK", &vp, mem_string, &memory, &start)) == 4) ||
        ((read = sscanf(&lines[i][0], "vep %u in %s have 0x%xK public memory starting at 0x%xK", &vp, mem_string, &memory, &start)) == 4)) {
      vep = vp;
      memory *= 1024;
      start *= 1024;
      public = 1;
      check_shared_mem = 1;
      if (0) fprintf(stderr,"> vep %u in %s have %u public memory starting at %u\n", vp, mem_string, memory, start);
    } else if (((read = sscanf(&lines[i][0], "vep %u in %s have %u public memory starting at %u", &vp, mem_string, &memory, &start)) == 4) ||
               ((read = sscanf(&lines[i][0], "vep %u in %s have 0x%x public memory starting at 0x%x", &vp, mem_string, &memory, &start)) == 4)) {
      if (0) fprintf(stderr,"> vep %u in %s have %u public memory starting at %u\n", vp, mem_string, memory, start);
      vep = vp;
      public = 1;
      check_shared_mem = 1;
    } else if (((read = sscanf(&lines[i][0], "vep %u in %s have %uK private memory starting at %uK", &vp, mem_string, &memory, &start)) == 4) ||
               ((read = sscanf(&lines[i][0], "vep %u in %s have 0x%xK private memory starting at 0x%xK", &vp, mem_string, &memory, &start)) == 4)) {
      vep = vp;
      memory *= 1024;
      start *= 1024;
      check_shared_mem = 1;
      if (0) fprintf(stderr,"> vep %u in %s have %u private memory starting at %u\n", vp, mem_string, memory, start);
    } else if (((read = sscanf(&lines[i][0], "vep %u in %s have %u private memory starting at %u", &vp, mem_string, &memory, &start)) == 4) ||
               ((read = sscanf(&lines[i][0], "vep %u in %s have 0x%x private memory starting at 0x%x", &vp, mem_string, &memory, &start)) == 4)) {
      if (0) fprintf(stderr,"> vep %u in %s have %u private memory starting at %u\n", vp, mem_string, memory, start);
      vep = vp;
      check_shared_mem = 1;
    } else if ((read = sscanf(&lines[i][0], "vep %u on tile %u partition %u has %uK stack in %uK memory starting at %uK", &vp, &tile, &partition, &stack, &memory, &start)) == 6) {
      if (0) fprintf(stderr,"> vep %u on tile %u partition %u has %uK stack in %uK memory starting at %uK\n", vp, tile, partition, stack, memory, start);
      check_mem = 1;
    } else if ((read = sscanf(&lines[i][0], "vep %u on tile %u next slot is for partition %u with %u cycles", &vp, &tile, &partition, &cycles)) == 4) {
      // keep for testing since corresponds directly to how the TDM table is actually programmed
      if (0) fprintf(stderr,"> vep %u on tile %u next slot is for partition %u with %u cycles\n", vp, tile, partition, cycles);
      check_slot_list = 1;
      if (slot_list == -1 || slot_list == 1) slot_list = 1;
      else {
        fprintf(stderr,"error: cannot mix different slot specifications: %s", &lines[i][0]);
        error =  1;
      }
    } else if ((read = sscanf(&lines[i][0], "vep %u on tile %u partition %u has %u cycles of %u starting at %u", &vp, &tile, &partition, &cycles, &period, &start)) == 6) {
      if (0) fprintf(stderr,"> vep %u on tile %u slot partition %u has %u cycles of %u starting at %u\n", vp, tile, partition, cycles, period, start);
      check_slot_abs = 1;
      if (slot_list == -1 || slot_list == 0) slot_list = 0;
      else {
        fprintf(stderr,"error: cannot mix different slot specifications: %s", &lines[i][0]);
        error =  1;
      }
    } else {
      fprintf(stderr, "error: cannot parse: %s", &lines[i][0]);
      error = 1;
    }
    if (vp == 0 || vp >= NUM_VEPS) { fprintf(stderr, "error: vep (%d) must between 1 and %d: %s", vp, NUM_VEPS, &lines[i][0]); error = 1; }
    if (tile >= NUM_TILES) { fprintf(stderr, "error: tile must between 0 and %d: %s", NUM_TILES, &lines[i][0]); error = 1; }
    if (partition > NUM_PARTITIONS) { fprintf(stderr, "error: partition must between 0 and %d (inclusive): %s", NUM_PARTITIONS, &lines[i][0]); error = 1; }
    if (check_mem) {
      if (memory != 4 && memory != 8 && memory != 16 && memory != 32 && memory != 64) { fprintf(stderr, "error: memory must be 4K, 8K, 16K, 32K, or 64K: %s", &lines[i][0]); error = 1; }
      if (ID_MEM_SIZE % memory != 0 || start < 32) { fprintf(stderr, "error: start must divide memory size (%dK) and not be less than 32K: %s", ID_MEM_SIZE, &lines[i][0]); error = 1; }
      if (stack >= memory) { fprintf(stderr, "error: stack must less than the memory: %s", &lines[i][0]); error = 1; }
      if (alloc_memory[tile][partition] != 0) { fprintf(stderr, "error: already allocated: %s", &lines[i][0]); error = 1; }
      if (!error && partition != 0) {
        // memory & stack of system application cannot be changed by user
        alloc_memory[tile][partition] = memory;
        alloc_memory_start[tile][partition] = start;
        alloc_stack[tile][partition] = stack;
      }
      in_vep[tile][partition] = vp;
    }
    if (check_slot_list) {
      if (slot[tile] >= NUM_SLOTS-1) { fprintf(stderr, "error: maximum number of slots per tile is %d: %s", NUM_SLOTS-1, &lines[i][0]); error = 1; }
      else if (cycles <= 0) { fprintf(stderr, "error: cycles must be greater than 0: %s", &lines[i][0]); error = 1; }
      else if (alloc_slot[tile][slot[tile]] != 0) { fprintf(stderr, "error: already allocated: %s", &lines[i][0]); error = 1; }
      else {
        // user is allowed to allocate extra slots for the system application
        alloc_slot[tile][slot[tile]] = partition;
        alloc_slot_length[tile][slot[tile]] = cycles;
        slot[tile]++;
      }
      in_vep[tile][partition] = vp;
    }
    if (check_slot_abs) {
      if (slot[tile] >= NUM_SLOTS-1) { fprintf(stderr, "error: maximum number of slots per tile is %d: %s", NUM_SLOTS-1, &lines[i][0]); error = 1; }
      else if (cycles > period - 2*VKERNEL_CYCLES - SYS_APP_CYCLES ||
               start > period - cycles - VKERNEL_CYCLES) { fprintf(stderr, "error: period is too small: %s", &lines[i][0]); error = 1; }
      else if (alloc_period[tile] != 0 && alloc_period[tile] != period) { fprintf(stderr, "error: must have 1 period per tile (%d != %d): %s", alloc_period[tile], period, &lines[i][0]); error = 1; }
      else if (alloc_slot[tile][slot[tile]] != 0) { fprintf(stderr, "error: already allocated: %s", &lines[i][0]); error = 1; }
      else {
        // user is allowed to allocate extra slots for the system application
        alloc_slot[tile][slot[tile]] = partition;
        alloc_slot_start[tile][slot[tile]] = start;
        alloc_slot_length[tile][slot[tile]] = cycles;
        alloc_period[tile] = period;
        slot[tile]++;
      }
      in_vep[tile][partition] = vp;
    }
    if (check_shared_mem) {
      mem_index = my_get_shared_mem_index(mem_string);
      if (vep >= NUM_VEPS) { fprintf(stderr, "error: maximum vep # is %d: %s", NUM_VEPS, &lines[i][0]); error = 1; }
      else if (mem_index == -1) { fprintf(stderr, "error: invalid shared memory: %s", &lines[i][0]); error = 1; }
      else if (memory < MIN_APP_SHARED_MEMORY || memory > SHARED_MEMORY_SIZE) { fprintf(stderr, "error: cannot allocate less than %u bytes or more than %dK in shared memory: %s", MIN_APP_SHARED_MEMORY, SHARED_MEMORY_SIZE, &lines[i][0]); error = 1; }
      else if (!power2(memory)) { fprintf(stderr, "error: shared memory allocation must be a power of 2: %s", &lines[i][0]); error = 1; }
      else if (start + memory > SHARED_MEMORY_SIZE) { fprintf(stderr, "error: allocation outside shared memory range: %s", &lines[i][0]); error = 1; }
      else if (public && alloc_shmem[mem_index][PUBLIC][vep] != 0) { fprintf(stderr, "error: already allocated: %s", &lines[i][0]); error = 1; }
      else if (!public && alloc_shmem[mem_index][PRIVATE][vep] != 0) { fprintf(stderr, "error: already allocated: %s", &lines[i][0]); error = 1; }
      else {
        if (public) {
          if (alloc_shmem[mem_index][PUBLIC][vep]) { fprintf(stderr, "error: can only allocate public shared memory once: %s", &lines[i][0]); error = 1; }
         else {
            alloc_shmem[mem_index][PUBLIC][vep] = memory;
            alloc_shmem_start[mem_index][PUBLIC][vep] = start;
          }
       } else {
          if (alloc_shmem[mem_index][PRIVATE][vep]) { fprintf(stderr, "error: can only allocate shared memory once: %s", &lines[i][0]); error = 1; }
         else {
            alloc_shmem[mem_index][PRIVATE][vep] = memory;
            alloc_shmem_start[mem_index][PRIVATE][vep] = start;
          }
        }
      }
    }
  }
  for (int t=0; t < NUM_TILES; t++) {
    for (unsigned int s=0; s < slot[t]; s++) {
      if(alloc_slot[t][s] && !alloc_memory[t][alloc_slot[t][s]]) {
        fprintf(stderr, "error: partition %d on tile %d has slots but no memory allocated\n", alloc_slot[t][s], t);
        error = 1;
      }
    }
  }
  for (int t=0; t < NUM_TILES; t++) {
    int sum = 0;
    for (int p=0; p <= NUM_PARTITIONS; p++) {
      sum += alloc_memory[t][p];
      if (!alloc_memory[t][p]) continue;
      for (int p2=0; p2 < p; p2++) {
        if (!alloc_memory[t][p2]) continue;
        if ((alloc_memory_start[t][p] >= alloc_memory_start[t][p2] &&
             alloc_memory_start[t][p] < alloc_memory_start[t][p2] + alloc_memory[t][p2]) ||
            (alloc_memory_start[t][p] + alloc_memory[t][p] >= alloc_memory_start[t][p2] &&
             alloc_memory_start[t][p] + alloc_memory[t][p] < alloc_memory_start[t][p2] + alloc_memory[t][p2])) {
          fprintf(stderr, "error: partitions %d and %d have overlapping allocations in instruction/data memory on tile %u (%uK-%uK, %uK-%uK)\n",
                  p, p2, t,
                  alloc_memory_start[t][p], alloc_memory_start[t][p] + alloc_memory[t][p],
                  alloc_memory_start[t][p2], alloc_memory_start[t][p2] + alloc_memory[t][p2]);
        }
      }
    }
    if (sum > ID_MEM_SIZE) {
      fprintf(stderr, "error: too much memory (%dK) allocated to memory mem%d\n", sum, t);
      error = 1;
    }
  }
  for (int m=0; m < NUM_SHARED_MEMORIES; m++) {
    int sum = 0;
    for (int p=0; p < 2; p++) {
      for (int v=0; v < NUM_VEPS; v++) {
        if (!alloc_shmem[m][p][v]) continue;
        sum += alloc_shmem[m][p][v];
        for (int p2=0; alloc_shmem[m][p][v] != 0 && p2 < 2; p2++) {
          if (!alloc_shmem[m][p2][v]) continue;
          for (int v2=0; v2 <= v; v2++) {
            // check for overlapping allocations of all public & private memory regions
            if (!alloc_shmem[m][p2][v2]) continue;
            if (v != v2 && 
                ((alloc_shmem_start[m][p][v] >= alloc_shmem_start[m][p2][v2] &&
                  alloc_shmem_start[m][p][v] < alloc_shmem_start[m][p2][v2] + alloc_shmem[m][p2][v2]) ||
                 (alloc_shmem_start[m][p][v] + alloc_shmem[m][p][v] >= alloc_shmem_start[m][p2][v2] &&
                  alloc_shmem_start[m][p][v] + alloc_shmem[m][p][v] < alloc_shmem_start[m][p2][v2] + alloc_shmem[m][p2][v2]))) {
              fprintf(stderr, "error: veps %d and %d have overlapping allocations in memory %s (0x%X-0x%X, 0x%X-0x%X)\n",
                      v2, v, shared_mems[m],
                      alloc_shmem_start[m][p][v2], alloc_shmem_start[m][p][v2] + alloc_shmem[m][p][v2],
                      alloc_shmem_start[m][p][v], alloc_shmem_start[m][p][v] + alloc_shmem[m][p][v]);
              error = 1;
            }
           // also check that after relocating the private region to 0 in the shared mem that it doesn't clash with any public region
            if (p == PRIVATE && p2 == PUBLIC &&
                alloc_shmem[m][p][v] > alloc_shmem_start[m][p2][v2]) {
              fprintf(stderr, "error: veps %d and %d have overlapping allocations in memory %s (0x%X-0x%X, logical 0x%X-0x%X)\n",
                      v2, v, shared_mems[m],
                      alloc_shmem_start[m][p2][v2], alloc_shmem_start[m][p2][v2] + alloc_shmem[m][p2][v2],
                      0, 0 + alloc_shmem[m][p][v]);
              error = 1;
            }
          }
        }
      }
    }
    if (sum > SHARED_MEMORY_SIZE) {
      fprintf(stderr, "error: too much memory (%dK) allocated to shared memory %s\n", sum, shared_mems[m]);
      error = 1;
    }
  }
  if (error) {
    fprintf(stderr, "abort with errors\n");
    return 1;
  }

  if (debug) {
    for (int t=0; t < NUM_TILES; t++) {
      for (int p=0; p <= NUM_PARTITIONS; p++) {
        if (in_vep[t][p]) fprintf(stderr, "in_vep[%d][%d]=%d\n", t, p, in_vep[t][p]);
      }
    }
    for (int t=0; t < NUM_TILES; t++) {
      for (int p=0; p <= NUM_PARTITIONS; p++) {
        if (alloc_memory[t][p]) fprintf(stderr, "memory[%d][%d]=%d\n", t, p, alloc_memory[t][p]);
      }
    }
    for (int t=0; t < NUM_TILES; t++) {
      for (int p=0; p <= NUM_PARTITIONS; p++) {
        if (alloc_memory_start[t][p]) fprintf(stderr, "memory_start[%d][%d]=%d\n", t, p, alloc_memory_start[t][p]);
      }
    }
    for (int t=0; t < NUM_TILES; t++) {
      for (int p=0; p <= NUM_PARTITIONS; p++) {
        if (alloc_stack[t][p]) fprintf(stderr, "stack[%d][%d]=%d\n", t, p, alloc_stack[t][p]);
      }
    }
    for (int t=0; t < NUM_TILES; t++) {
      for (unsigned int s=0; s < slot[t]; s++) {
        fprintf(stderr, "slot[%d][%d]=%d\n", t, s, alloc_slot[t][s]);
      }
    }
    if (slot_list == 0) {
      for (int t=0; t < NUM_TILES; t++) {
        for (unsigned int s=0; s < slot[t]; s++) {
          fprintf(stderr, "start[%d][%d]=%d\n", t, s, alloc_slot_start[t][s]);
        }
      }
    }
    for (int t=0; t < NUM_TILES; t++) {
      for (unsigned int s=0; s < slot[t]; s++) {
        fprintf(stderr, "length[%d][%d]=%d\n", t, s, alloc_slot_length[t][s]);
      }
    }
    for (int t=0; t < NUM_TILES; t++) {
      fprintf(stderr, "period[%d]=%d\n", t, alloc_period[t]);
    }
    for (int m=0; m < NUM_SHARED_MEMORIES; m++) {
      for (int p=0; p < 2; p++) {
        for (int v=0; v < NUM_VEPS; v++) {
          fprintf(stderr, "shmem[%d][%d][%d]=%d\n", m, p, v, alloc_shmem[m][p][v]);
        }
      }
    }
    for (int m=0; m < NUM_SHARED_MEMORIES; m++) {
      for (int p=0; p < 2; p++) {
        for (int v=0; v < NUM_VEPS; v++) {
          fprintf(stderr, "shmem_start[%d][%d][%d]=%d\n", m, p, v, alloc_shmem_start[m][p][v]);
        }
      }
    }
  }

  ///// generate slot list from absolute slots /////
  if (ls_tile == -1 && slot_list == 0) {
    // sort slots on starting time
    int alloc_slot2[NUM_TILES][NUM_SLOTS+1] = { 0 };
    int alloc_slot_start2[NUM_TILES][NUM_SLOTS+1] = { 0 };
    int alloc_slot_length2[NUM_TILES][NUM_SLOTS+1] = { 0 };
    for (int t=0; t < NUM_TILES; t++) {
      // temporarily add a slot to correspond to the specified period
      if (alloc_period[t] == 0) {
        // we didn't assigned any slots on this tile, only have one system application slot
        continue;
      }
      alloc_slot_start[t][slot[t]] = alloc_period[t];
      slot[t]++;
      unsigned int slots = 0;
      while (slots < slot[t]) {
        unsigned int smallest = ~0, si = -1;
        for (unsigned int s=0; s < slot[t]; s++) {
          if (alloc_slot_start[t][s] <= smallest) {
            smallest = alloc_slot_start[t][s];
            si = s;
         }
       }
        alloc_slot2[t][slots] = alloc_slot[t][si];
        alloc_slot_start2[t][slots] = alloc_slot_start[t][si];
        alloc_slot_length2[t][slots] = alloc_slot_length[t][si];
        alloc_slot_start[t][si] = ~0 -1;
        slots++;
      }
      // check that slot 0 is for system application
      // assume that we start with application slot, followed by a VKERNEL slot
      // check for overlapping slots & gaps, including the VKERNEL
      if (alloc_slot_start2[t][0] != 0) {
        fprintf(stderr, "error: tile %d slot 0 is misaligned (should start at %d cycles)\n", t, VKERNEL_CYCLES);
        error = 1;
      }
      if (alloc_slot2[t][0] != 0) {
        fprintf(stderr, "error: tile %d slot 0 isn't for the system application\n", t);
        error = 1;
      }
      for (unsigned int s=1; s < slots; s++) {
        int diff = alloc_slot_start2[t][s] - (alloc_slot_start2[t][s-1] + alloc_slot_length2[t][s-1] + VKERNEL_CYCLES);
       if (diff == 0) { /* nothing to do */ ; }
       else if (diff < 0) {
          fprintf(stderr, "error: tile %d slot %d finishes at %d which is after the next slot/period starts at %d\n",
                  t, s, alloc_slot_start2[t][s-1] + alloc_slot_length2[t][s-1] + VKERNEL_CYCLES, alloc_slot_start2[t][s]);
          error = 1;
        }
       else if (diff < 2*VKERNEL_CYCLES) {
          // padding slot must be at least VKERNEL cycles (arbitrary choice)
          fprintf(stderr, "error: tile %d slot %d finishes at %d which is too close to the start of the next slot/period %d (must >= %d cycles)\n",
                  t, s, alloc_slot_start2[t][s-1] + alloc_slot_length2[t][s-1] + VKERNEL_CYCLES, alloc_slot_start2[t][s], 2*VKERNEL_CYCLES);
          error = 1;
       }
       else if (slot[t] == NUM_SLOTS +1) { // +1 for temporary extra slot
          fprintf(stderr, "error: tile %d ran out of slots, cannot insert padding slot after slot %d\n", t, s);
          error = 1;
       }
       else {
          // insert padding slot for the system application, shifting everything up
          if (debug) fprintf(stderr, "info: insert padding tile %d slot %d from %d to %d\n",
                             t, s, alloc_slot_start2[t][s-1] + alloc_slot_length2[t][s-1] + VKERNEL_CYCLES, alloc_slot_start2[t][s]);
          for (unsigned int x = slots; x >= s; x--) {
            alloc_slot2[t][x] = alloc_slot2[t][x-1];
            alloc_slot_start2[t][x] = alloc_slot_start2[t][x-1];
            alloc_slot_length2[t][x] = alloc_slot_length2[t][x-1];
          }
          alloc_slot2[t][s] = 0;
          alloc_slot_start2[t][s] = alloc_slot_start2[t][s-1] + alloc_slot_length2[t][s-1] + VKERNEL_CYCLES;
          alloc_slot_length2[t][s] = diff - VKERNEL_CYCLES;
         slots++;
         s--;
       }
      }
      for (unsigned int s=0; s < slots; s++) {
        alloc_slot[t][s] = alloc_slot2[t][s];
        alloc_slot_start[t][s] = alloc_slot_start2[t][s];
        alloc_slot_length[t][s] = alloc_slot_length2[t][s];
      }
      // remove the extra 'period' slot
      slot[t] = slots -1;
    }
    if (error || printtdm) {
      for (int t=0; t < NUM_TILES; t++) {
        unsigned int sum = 0;
        for (unsigned int s=0; s < slot[t]; s++) {
          fprintf(stderr, "tile %d slot %2d from %6d length %6d partition %2d of vep %d\n",
                           t, s, sum, alloc_slot_length[t][s], alloc_slot[t][s], in_vep[t][alloc_slot[t][s]]);
          sum += alloc_slot_length[t][s] + VKERNEL_CYCLES;
        }
      }
    }
  }

  if (error) {
    fprintf(stderr, "abort with errors\n");
    return 1;
  }

  ///// generate lscript.ld for partition ////
  if (ls_tile != -1) {
    printf("/* DO NOT EDIT\n"
           " * this file was automatically generated from %s for\n"
           " * VEP_ID=%d TILE_ID=%d PARTITION_ID=%d\n"
           " */\n",
           pgm, ls_vep, ls_tile, ls_partition);
    printf("MEMORY {\n");
    printf("mem : ORIGIN = 0x0, LENGTH = 0x%08X\n", alloc_memory[ls_tile][ls_partition]*1024);
    // add the public regions of all veps (incl. self)
    // all shared regions of all veps in all memories are remapped to this single region
    // see explanation of memory map at tile-mmu-add
    printf("publicmem : ORIGIN = 0x%08X, LENGTH = 0x%08X /* vep=%d tile=%d public regions of all veps */\n",
           PUBLIC_VEP_REGION_START, NUM_SHARED_MEMORIES_POW2 * NUM_VEPS * PUBLIC_VEP_REGION_SIZE,
           ls_vep, ls_tile);
    for (int m=0; m < NUM_SHARED_MEMORIES; m++) {
      if (alloc_shmem[m][PRIVATE][ls_vep] != 0 && shared_mems_start[ls_tile][m] != -1) {
        printf("%s : ORIGIN = 0x%08X, LENGTH = 0x%08X /* vep=%d tile=%d partition=%d private memory */\n",
               shared_mems[m], shared_mems_start[ls_tile][m], alloc_shmem[m][PRIVATE][ls_vep],
               ls_vep, ls_tile, ls_partition);
      }
      if (alloc_shmem[m][PUBLIC][ls_vep] != 0 && shared_mems_start[ls_tile][m] != -1) {
        // rerun.sh needs to know the physical addresses of the public region to zero it
        printf("/* %s : ORIGIN = 0x%08X, LENGTH = 0x%08X vep=%d tile=%d partition=%d public memory */\n",
               shared_mems[m], shared_mems_start[ls_tile][m] + alloc_shmem[m][PUBLIC][ls_vep],
               PUBLIC_VEP_REGION_SIZE, ls_vep, ls_tile, ls_partition);
      }
    }
    printf("}\n");
    printf("_STACK_SIZE = 0x%08X;\n", alloc_stack[ls_tile][ls_partition]*1024);
    printf(
"OUTPUT_ARCH( \"riscv\" )\n"
"ENTRY( _start )\n"
"_STACK_SIZE = DEFINED(_STACK_SIZE) ? _STACK_SIZE : 0x1000;\n"
"SECTIONS\n"
"{\n"
"  . = 0x00000000;\n"
"  .text.init : { *(.text.init) } > mem\n"
"  .text : { *(.text) } > mem\n"
"  .data : { *(.data) } > mem\n"
"  .bss.align :\n"
"   {\n"
"           . = ALIGN(4);\n"
"           _bss = .;\n"
"   } > mem\n"
"   .bss.start :\n"
"   {\n"
"          _bss_lma = LOADADDR(.bss.start);\n"
"   } >mem\n"
"   .bss :\n"
"   {\n"
"           *(.sbss*)\n"
"           *(.gnu.linkonce.sb.*)\n"
"           *(.bss .bss.*)\n"
"           *(.gnu.linkonce.b.*)\n"
"           *(COMMON)\n"
"        . = ALIGN(4);\n"
"        _ebss = .;\n"
"   } >mem\n"
);
    for (int m=0; m < NUM_SHARED_MEMORIES; m++) {
      if (alloc_shmem[m][PRIVATE][ls_vep] != 0 && shared_mems_start[ls_tile][m] != -1)
        printf("  .%s : { *(.%s) } > %s\n", shared_mems[m], shared_mems[m], shared_mems[m]);
    }
    printf(
"  _end = .;\n"
"  .stack (NOLOAD) : {\n"
"          _stack_end = .;\n"
"          . += _STACK_SIZE;\n"
"          . = ALIGN(4);\n"
"          _stack_start = .;\n"
"  } > mem\n"
"  PROVIDE (end = .);\n"
"}\n"
);


  ///// generate commands for dynload /////
  }
  if (json) {
    // see which applications to not schedule
    // cannot suspend application 0 (in any slot) on any tile
    int i = 1;
    unsigned int vep, tile, part;
    if (argc > 0 && argc < 4) error = 1;
    while (!error && i < argc-1) {
      if (sscanf(argv[i],"%u", &vep) == 1 && sscanf(argv[i+1],"%u", &tile) == 1 && sscanf(argv[i+2],"%u", &part) == 1) {
        if (vep == 0 || vep >= NUM_VEPS || tile >= NUM_TILES || part <= 0 || part > NUM_PARTITIONS) {
          fprintf(stderr, "error: invalid vep %u or tile %u or partition %u\n", vep, tile, part);
          error = 1;
          break;
        }
        int suspended = 0;
        for (unsigned int s=1; s < slot[tile]; s++) {
          if (alloc_slot[tile][s] == part) {
            fprintf(stderr, "info: suspend vep %u tile %u partition %u slot %u\n", vep, tile, part, s);
            alloc_slot[tile][s] = 0;
            suspended = 1;
          }
        }
        i += 3;
        if (!suspended)
          fprintf(stderr, "info: did not suspend vep %u tile %u partition %u\n", vep, tile, part);
      }
    }
    if (error || (argc > 2 && i != argc)) {
      USAGE;
      fprintf(stderr, "abort with errors\n");
      return 1;
    }

    // standard platform
    // the 10000000 doesn't matter (it's the total TDM length, it just needs to be large enough)
    printf(
        "tile-add    0 40000000 10000000 128k 32 16\n"
        "tile-add    1 40000000 10000000 128k 32 16\n"
        "tile-add    2 40000000 10000000 128k 32 16\n"
        "tile-tdm-os 0 %d\n"
        "tile-tdm-os 1 %d\n"
        "tile-tdm-os 2 %d\n"
        "tile-mem-os 0 %dk\n"
        "tile-mem-os 1 %dk\n"
        "tile-mem-os 2 %dk\n",
        VKERNEL_CYCLES, VKERNEL_CYCLES, VKERNEL_CYCLES,
        SYS_APP_MEMORY, SYS_APP_MEMORY, SYS_APP_MEMORY
        );

    if (!power2(NUM_SHARED_MEMORIES_POW2) || NUM_SHARED_MEMORIES > NUM_SHARED_MEMORIES_POW2) {
      fprintf(stderr, "internal error: NUM_SHARED_MEMORIES_POW2 must be a power of two but it is %d\n", NUM_SHARED_MEMORIES_POW2);
      exit(1);
    }
    int sum[NUM_TILES] = { 0 };
    if (printmm) {
      fprintf(stderr, "memory mapping from shared memories into per-partition memory map:\n");
      fprintf(stderr, "vep v's public region in memory m is mapped starting at 0x%08X + (v * %d + (index of memory m)) * %dK\n",
                      PUBLIC_VEP_REGION_START, NUM_SHARED_MEMORIES_POW2, PUBLIC_VEP_REGION_SIZE/1024);
      fprintf(stderr, "shared memory indexes starting at 0: [");
      for (unsigned int m=0; m < NUM_SHARED_MEMORIES; m++)
        fprintf(stderr, "%s%s", shared_mems[m], (m == NUM_SHARED_MEMORIES-1 ? "]\n" : ","));
    }
    for (int t=0; t < NUM_TILES; t++) {
      for (int p=1; p <= NUM_PARTITIONS; p++) {
        if (alloc_memory[t][p] != 0) {
          int s = 0;
          printf("tile-mem-add  %d %d %dk 0x%X\n", t, p, alloc_memory[t][p], alloc_memory_start[t][p]*1024);
          // we don't check if the vep directory actually exists; dynload will silently ignore
          printf("tile-mem-data %d %d vep_%d/partition_%d_%d/out.hex\n", t, p, in_vep[t][p], t, p);
          // we don't lock the application in the mapping
          // note that this means that they may be moved around when adding new applications
          // when dynamically loading; there's currently no good way to deal with this
          //printf("tile-mem-lock %d %d\n", t, p);
          printf("tile-mmu-add  %d %d %d 0x80FC0000 0x80FC0000 0xFFFEFFF8\n", t, p, s++);
//          printf("tile-mmu-add  %d %d %d 0x80FC0000 0x80FC0000 0xFFFEFFF8\n", t, p, s++);
	  printf("tile-mmu-add  %d %d %d 0x81000000 0x81000000 0xff000000\n", t, p, s++);
          shared_memory_map *map = (shared_memory_map *) PLATFORM_OCM_MEMORY_LOC;
          printf("tile-mmu-add  %d %d %d 0x%08X 0x%08X 0xFFFFFC00\n",
                 t, p, s++, PLATFORM_OCM_MEMORY_LOC, (unsigned int) &(map->vps[p-1]));
          for (unsigned int m=0; m < NUM_SHARED_MEMORIES; m++) {
            for (unsigned int v=0; v < NUM_VEPS; v++) {
              // open up private memory of vep's partition
              if (in_vep[t][p] == v && alloc_shmem[m][PRIVATE][v] != 0 && shared_mems_start[t][m] != -1) {
                const uint32_t mask = ~(alloc_shmem[m][PRIVATE][v] -1);
                // or could use: const uint32_t mask = ~(PUBLIC_VEP_REGION_SIZE-1);
                printf("tile-mmu-add  %d %d %d 0x%08X 0x%08X 0x%08X\n",
                       t, p, s++, shared_mems_start[t][m], shared_mems_start[t][m] + alloc_shmem_start[m][PRIVATE][v], mask);
                if (printmm) fprintf(stderr, "on tile %d partition %d entry %2d map private region of vep %d in %s to 0x%08X         from 0x%0X (+%3dK) with mask 0x%08X (%dK)\n",
                       t, p, s, v, shared_mems[m],
                     shared_mems_start[t][m], shared_mems_start[t][m] + alloc_shmem_start[m][PRIVATE][v], 
                     alloc_shmem_start[m][PRIVATE][v]/1024, mask, alloc_shmem[m][PRIVATE][v]/1024);
             }
              if (s >= NUM_MMU_ENTRIES) {
                fprintf(stderr, "error: maximum # MMU entries %d\n", s);
                exit(1);
             }
           }
         }
          for (unsigned int m=0; m < NUM_SHARED_MEMORIES; m++) {
            for (unsigned int v=0; v < NUM_VEPS; v++) {
             // each vep can have a public region of at most PUBLIC_VEP_REGION_ size in every shared memory it can access
             // memory map: vep i's public region is mapped starting at 0x8200 0000 + (i * NUM_SHARED_MEMORIES_POW2 + (index of shared memory)) * PUBLIC_VEP_REGION_SIZE
             // NUM_VEPS must be a power of 2 (although strictly speaking that is not necessary)
             // since not every core can reach every memory this is a sparse encoding, with unused addresses
              if (alloc_shmem[m][PUBLIC][v] != 0 && shared_mems_start[t][m] != -1) {
               const uint32_t mask = ~(PUBLIC_VEP_REGION_SIZE-1);
                printf("tile-mmu-add  %d %d %d 0x%08X 0x%08X 0x%08X\n",
                       t, p, s++,
                     // place in memory map for everyone:
                     PUBLIC_VEP_REGION_START + (v * NUM_SHARED_MEMORIES_POW2 + m) * PUBLIC_VEP_REGION_SIZE,
                     // place in physical memory:
                     shared_mems_start[t][m] + alloc_shmem_start[m][PUBLIC][v], mask);
                if (printmm) fprintf(stderr, "on tile %d partition %d entry %2d map  public region of vep %d in %s to 0x%08X (+%3dK) from 0x%0X (+%3dK) with mask 0x%08X (%dK)\n",
                       t, p, s, v, shared_mems[m],
                     // place in memory map for everyone:
                     PUBLIC_VEP_REGION_START + (v * NUM_SHARED_MEMORIES_POW2 + m) * PUBLIC_VEP_REGION_SIZE,
                     ((v * NUM_SHARED_MEMORIES + m) * PUBLIC_VEP_REGION_SIZE)/1024,
                     // place in physical memory:
                     shared_mems_start[t][m] + alloc_shmem_start[m][PUBLIC][v], 
                     alloc_shmem_start[m][PUBLIC][v]/1024, mask, PUBLIC_VEP_REGION_SIZE/1024);
                if (s >= NUM_MMU_ENTRIES) {
                  fprintf(stderr, "error: maximum # MMU entries %d (too many public vep regions?)\n", s);
                  exit(1);
               }
             }
           }
         }
        }
      }
      for (unsigned int s=0; s < slot[t]; s++) {
        sum[t] += alloc_slot_length[t][s] + VKERNEL_CYCLES;
        // old non-zero slots are cleared by dynload
        if (alloc_slot_length[t][s])
          printf("tile-tdm-add  %d %d %d\n", t, alloc_slot[t][s], alloc_slot_length[t][s]);
      }
    }
    fprintf(stderr, "info: slot table lengths are: ");
    for (int t=0; t < NUM_TILES; t++) fprintf(stderr,"%d ", sum[t]);
    fprintf(stderr,"cycles\n");
  }
}
