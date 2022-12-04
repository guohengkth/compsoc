#ifndef __VEP_PRIVATE_REGION_H__
#define __VEP_PRIVATE_REGION_H__

#include <stddef.h>
#include <stdint.h>

// IMPORTANT: to use a private memory region you must:
// 1- typedef the struct containing all data to be placed in the region
// 2- uncomment the relevant #define USE_VEP_PRIVATE_MEM* below
//    this will declare & initialise the vep_private_memXY external variables
// 3- declare the memory region in the vep-config.txt file
//    without this, the region will not be declared in the memory map
// 
// all fields are set to 0 when the vep is loaded
// (it may therefore be used to have an 'initialized' as first field in the struct,
// this allows other veps can check if this vep is running and initialized or not)
// 
// the private data of this vep on tile T in memory M is mapped to VEP_PRIVATE_REGION_TILE<T>_MEM<M>_START

typedef volatile struct {
  uint32_t initialized;
  // more fields
} vep_private_mem01_t;
#define USE_VEP_PRIVATE_MEM01

typedef volatile struct {
  uint32_t initialized;
  // more fields
} vep_private_mem02_t;
// #define USE_VEP_PRIVATE_MEM02

typedef volatile struct {
  uint32_t initialized;
  // more fields
} vep_private_mem12_t;
// #define USE_VEP_PRIVATE_MEM12


/***** DO NOT MODIFY CODE BELOW *****/

#ifdef USE_VEP_PRIVATE_MEM01
#if(TILE_ID == 0 || TILE_ID == 1)
// the private structure will be accessible via this pointer (initialized in the .c file)
// it is mapped to VEP_PRIVATE_REGION_TILE[01]_MEM01_START
extern volatile vep_private_mem01_t * const vep_private_mem01;
#endif
#endif

#ifdef USE_VEP_PRIVATE_MEM02
#if(TILE_ID == 0 || TILE_ID == 2)
// the private structure will be accessible via this pointer (initialized in the .c file)
// it is mapped to VEP_PRIVATE_REGION_TILE[02]_MEM02_START
extern volatile vep_private_mem02_t * const vep_private_mem02;
#endif
#endif


#ifdef USE_VEP_PRIVATE_MEM12
#if(TILE_ID == 1 || TILE_ID == 2)
// the private structure will be accessible via this pointer (initialized in the .c file)
// it is mapped to VEP_PRIVATE_REGION_TILE[12]_MEM12_START
extern volatile vep_private_mem12_t * const vep_private_mem12;
#endif
#endif

#endif
