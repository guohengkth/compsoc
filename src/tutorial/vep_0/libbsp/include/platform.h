#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#define NUM_TILES 3
#define NUM_VPS 8
#define NUM_PARTITIONS NUM_VPS /* alias */
#define NUM_VEPS 16 /* maximum number of veps, this may be more than NUM_VPS (the max. # running veps) */
#define NUM_SLOTS 32
#define NUM_MMU_ENTRIES 16

#define CHEAP_BUFFER_SIZE_SYS 768
#define CHEAP_BUFFER_SIZE 224 
//#define CHEAP_MAX_VP_SIZE 4096
#define PLATFORM_OCM_MEMORY_LOC 0x800C0000 /* from RISC-V */

#define VKERNEL_CYCLES 2000		/* cycles for the vkernel slot */
#define MAX_TDM_SLOTS 32		/* including at least one for system application */
#define SYS_APP_MEMORY 32		/* KB required by system application 0 */
#define SYS_APP_CYCLES 5000		/* first slot of this many cycles is reserved for the system application in the TDM table */
#define ID_MEM_SIZE 128			/* the RISC-V instruction/data memory */
#define DEFAULT_APP_STACK 4		/* KB */
#define DEFAULT_APP_SLOT_CYCLES 20000	/* per slot, currently there's no min (except >0) or max */
#define NUM_SHARED_MEMORIES 3
#define NUM_SHARED_MEMORIES_POW2 4	/* nr shared memories rounded up to nearest power of 2 */
#define SHARED_MEMORY_SIZE (32*1024)	/* B */
#define MIN_APP_SHARED_MEMORY 1024	/* B */
#define PUBLIC_VEP_REGION_SIZE (8*1024)	/* max size of a VEP region in a shared memory */
#define PUBLIC_VEP_REGION_START 0x82000000 /* start address of public regions in the memory map */

/***** memory map *****/

// start address of shared memory from a given tile
// these should be generated automatically
#define TILE0_MEM01_START 0x80020000
#define TILE0_MEM02_START 0x80030000
#define TILE1_MEM01_START 0x80020000
#define TILE1_MEM12_START 0x80030000
#define TILE2_MEM02_START 0x80020000
#define TILE2_MEM12_START 0x80030000

/* old names, do not use
#define TILE0_TIMER_0_S_AXI 0x80FC0000
#define TILE1_TIMER_0_S_AXI 0x80FC0000
#define TILE2_TIMER_0_S_AXI 0x80FC0000
#define TILE0_TIMER_1_S_AXI 0x80FD0000
#define TILE1_TIMER_1_S_AXI 0x80FD0000
#define TILE2_TIMER_1_S_AXI 0x80FD0000
*/

#define TILE0_GLOBAL_TIMER 0x80FC0000
#define TILE1_GLOBAL_TIMER 0x80FC0000
#define TILE2_GLOBAL_TIMER 0x80FC0000
#define TILE0_PARTITION_TIMER 0x80FD0000
#define TILE1_PARTITION_TIMER 0x80FD0000
#define TILE2_PARTITION_TIMER 0x80FD0000

#define TILE0_AXI_BRAM_CTRL_0_S_AXI 0x800C0000

// vep private regions are remapped to the start of the shared memory
#define VEP_PRIVATE_REGION_TILE0_MEM01_START TILE0_MEM01_START
#define VEP_PRIVATE_REGION_TILE0_MEM02_START TILE0_MEM02_START
#define VEP_PRIVATE_REGION_TILE1_MEM01_START TILE1_MEM01_START
#define VEP_PRIVATE_REGION_TILE1_MEM12_START TILE1_MEM12_START
#define VEP_PRIVATE_REGION_TILE2_MEM02_START TILE2_MEM02_START
#define VEP_PRIVATE_REGION_TILE2_MEM12_START TILE2_MEM12_START

// these should be generated automatically
#define SHARED_MEMORY_MEM01_INDEX 0
#define SHARED_MEMORY_MEM02_INDEX 1
#define SHARED_MEMORY_MEM12_INDEX 2
// start address of the vep's public region in a given shared memory (from any tile)
// note that not all shared memories are accessible from every tile
#define VEP_PUBLIC_REGION_START(vep,sharedmemindex) (PUBLIC_VEP_REGION_START + (vep * NUM_SHARED_MEMORIES_POW2 + sharedmemindex) * PUBLIC_VEP_REGION_SIZE)


/***** programmatically retrieve the memory map *****/

// names of all shared memories
extern const char shared_mems[NUM_SHARED_MEMORIES][10];
// list of memories reachable from a tile at a base address (-1 if not)
extern const int shared_mems_start[NUM_TILES][NUM_SHARED_MEMORIES/*shmem 01 02 12*/];

extern int get_shared_mem_index(const char * const mem);
extern int get_shared_mem_start(const char * const mem, const int tile);


#endif
