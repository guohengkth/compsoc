#ifndef __VEP_PUBLIC_SHARED_MEMORY_H__
#define __VEP_PUBLIC_SHARED_MEMORY_H__

#include <platform.h>

// IMPORTANT: to use a public memory region you must:
// 1- typedef the struct containing all data to be placed in the region
// 2- uncomment the relevant #define USE_VEP_PUBLIC_MEM* below
//    this will declare & initialise the vep_public_memXY external variables
// 3- declare the memory region in the vep-config.txt file
//    without this, the region will not be declared in the memory map
// 
// all fields are set to 0 when the vep is loaded
// (it may therefore be used to have an 'initialized' as first field in the struct,
// this allows other veps can check if this vep is running and initialized or not)
// 
// the public data of this vep memory M is mapped to VEP_PUBLIC_REGION_MEM<M>_START
// (note that not all memories may be accessible from all tiles)

typedef volatile struct {
  uint32_t initialized;
  // more fields
} vep_public_mem01_t;
// #define USE_VEP_PUBLIC_MEM01

typedef volatile struct {
  uint32_t initialized;
  // more fields
} vep_public_mem02_t;
// #define USE_VEP_PUBLIC_MEM02

typedef volatile struct {
  uint32_t initialized;
  // more fields
} vep_public_mem12_t;
// #define USE_VEP_PUBLIC_MEM12


/***** DO NOT MODIFY CODE BELOW *****/

#ifdef USE_VEP_PUBLIC_MEM01
#if(TILE_ID == 0 || TILE_ID == 1)
// the public data of this vep in memory mem01 is mapped to the following address
#define VEP_PUBLIC_REGION_MEM01_START (VEP_PUBLIC_REGION_START(VEP_ID,SHARED_MEMORY_MEM01_INDEX))
// the public structure will be accessible via this pointer (initialized in the .c file)
extern volatile vep_public_mem01_t * const vep_public_mem01;
#endif
#endif

#ifdef USE_VEP_PUBLIC_MEM02
#if(TILE_ID == 0 || TILE_ID == 2)
// the public data of this vep in memory mem02 is mapped to the following address
#define VEP_PUBLIC_REGION_MEM02_START (VEP_PUBLIC_REGION_START(VEP_ID,SHARED_MEMORY_MEM02_INDEX))
// the public structure will be accessible via this pointer (initialized in the .c file)
extern volatile vep_public_mem02_t * const vep_public_mem02;
#endif
#endif


#ifdef USE_VEP_PUBLIC_MEM12
#if(TILE_ID == 1 || TILE_ID == 2)
// the public data of this vep in memory mem12 is mapped to the following address
#define VEP_PUBLIC_REGION_MEM12_START (VEP_PUBLIC_REGION_START(VEP_ID,SHARED_MEMORY_MEM12_INDEX))
// the public structure will be accessible via this pointer (initialized in the .c file)
extern volatile vep_public_mem12_t * const vep_public_mem12;
#endif
#endif

#endif
